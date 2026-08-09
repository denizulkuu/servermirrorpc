#!/usr/bin/env python3
"""ESP32 Screen Mirror — Optimized Streaming Client
Features:
  - Binary 4-byte protocol (was text header)
  - Chroma 4:2:0 subsampling (30-40% smaller JPEGs)
  - Multi-threaded pipeline: capture → encode → send
  - Local & Remote modes (remote = cloud relay)
  - Editable IP/Port + Room token
Double-click to launch. No console needed.
"""
import sys, os, io, time, threading, queue, atexit, logging
from tkinter import Tk, Frame, Label, Button, Scale, StringVar, Entry, HORIZONTAL, W, E, ttk

logging.getLogger("websockets").setLevel(logging.ERROR)

try: import mss; from PIL import Image; import websockets, asyncio
except ImportError:
    import tkinter.messagebox; tkinter.messagebox.showerror("Missing", "pip install mss pillow websockets"); sys.exit(1)

try: import cv2, numpy as np; HAS_CV2 = True
except ImportError: HAS_CV2 = False

# ── Defaults ──────────────────────────────────────────────────────────────────
DEFAULT_IP     = "192.168.1.11"
DEFAULT_PORT   = 81
DEFAULT_Q      = 70
DEFAULT_FPS    = 20
FRAME_W        = 320
FRAME_H        = 180
MAX_WS_SIZE    = 4 * 1024 * 1024

# Remote relay
RELAY_HOST_DEFAULT = "mirror-relay.onrender.com"
RELAY_PORT_DEFAULT = 443
ROOM_DEFAULT       = "deniz-mirror"


class StreamEngine:
    """Thread-safe streaming engine with multi-threaded pipeline."""

    def __init__(self):
        self.running     = False
        self.mode        = "local"   # "local" or "remote"
        self.thread      = None
        self.q           = queue.Queue()
        self._mss        = None
        self._loop       = None
        # Pipeline queues
        self._raw_q      = queue.Queue(maxsize=4)    # raw screenshots
        self._jpeg_q     = queue.Queue(maxsize=4)    # encoded JPEGs

    # ── Capture thread ────────────────────────────────────────────────────────
    def _capture_loop(self, mon, fps_cap):
        min_interval = 1.0 / max(fps_cap, 1)
        last = 0.0
        while self.running:
            now = time.time()
            if now - last < min_interval:
                time.sleep(0.001)
                continue
            try:
                raw = self._mss.grab(mon)
                last = now
                # Drop if queue full (backpressure)
                if self._raw_q.full():
                    try: self._raw_q.get_nowait()
                    except queue.Empty: pass
                self._raw_q.put(raw, timeout=0.1)
            except (queue.Full, Exception):
                pass

    # ── Encode thread ─────────────────────────────────────────────────────────
    def _encode_loop(self, quality):
        while self.running:
            try:
                raw = self._raw_q.get(timeout=0.5)
            except queue.Empty:
                continue

            jpeg = None
            try:
                if HAS_CV2:
                    arr = np.frombuffer(raw.bgra, np.uint8)
                    arr = arr.reshape(raw.height, raw.width, 4)
                    bgr = cv2.cvtColor(arr, cv2.COLOR_BGRA2BGR)
                    res = cv2.resize(bgr, (FRAME_W, FRAME_H),
                                     interpolation=cv2.INTER_LINEAR)
                    # Chroma 4:2:0 subsampling → 30-40% smaller JPEG
                    params = [cv2.IMWRITE_JPEG_QUALITY, quality,
                              cv2.IMWRITE_JPEG_SAMPLING_FACTOR, cv2.IMWRITE_JPEG_SAMPLING_420]
                    ok, j = cv2.imencode('.jpg', res, params)
                    if ok:
                        jpeg = j.tobytes()
                else:
                    im = Image.frombytes("RGB", raw.size, raw.bgra, "raw", "BGRX")
                    im = im.resize((FRAME_W, FRAME_H), Image.Resampling.BILINEAR)
                    b = io.BytesIO()
                    im.save(b, "JPEG", quality=quality, subsampling=0)  # 0 = 4:2:0 in PIL
                    jpeg = b.getvalue()
            except Exception:
                continue

            if jpeg and len(jpeg) > 100:  # sanity: JPEG header ~600 bytes min, but be lenient
                if self._jpeg_q.full():
                    try: self._jpeg_q.get_nowait()
                    except queue.Empty: pass
                try:
                    self._jpeg_q.put(jpeg, timeout=0.1)
                except queue.Full:
                    pass

    # ── Send thread (async) ───────────────────────────────────────────────────
    def _send_loop(self, ip, port, relay_host, relay_port, room):
        self._loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self._loop)

        async def sender():
            if self.mode == "local":
                uri = f"ws://{ip}:{port}"
            else:
                uri = f"wss://{relay_host}:{relay_port}/?room={room}"

            frame  = 0
            t0     = time.time()
            last_fps_report = t0

            while self.running:
                try:
                    async with websockets.connect(
                        uri,
                        ping_interval=None,
                        max_size=MAX_WS_SIZE,
                        close_timeout=30
                    ) as ws:
                        self.q.put(("connected", uri))
                        frame = 0; t0 = time.time(); last_fps_report = t0

                        while self.running:
                            try:
                                jpeg = self._jpeg_q.get(timeout=0.2)
                            except queue.Empty:
                                await asyncio.sleep(0.001)
                                continue

                            if not jpeg:
                                continue

                            # Binary protocol: 4-byte LE length prefix + JPEG
                            jpeg_len = len(jpeg)
                            header = bytes([
                                jpeg_len & 0xFF,
                                (jpeg_len >> 8) & 0xFF,
                                (jpeg_len >> 16) & 0xFF,
                                (jpeg_len >> 24) & 0xFF,
                            ])
                            frame_data = header + jpeg

                            try:
                                await ws.send(frame_data)
                            except Exception:
                                break  # reconnect

                            frame += 1
                            now = time.time()
                            if now - last_fps_report >= 2.0 and frame > 0:
                                elapsed = now - t0
                                if elapsed > 0:
                                    self.q.put(("fps", frame / elapsed))
                                last_fps_report = now

                except asyncio.CancelledError:
                    break
                except Exception as e:
                    msg = str(e)
                    if "close frame" not in msg.lower() and "timeout" not in msg.lower():
                        self.q.put(("error", msg[:80]))
                    await asyncio.sleep(0.5)

        try:
            self._loop.run_until_complete(sender())
        finally:
            self._loop.close()

    # ── Public API ────────────────────────────────────────────────────────────
    def start(self, config):
        """config: dict with keys: ip, port, quality, fps, mode, relay_host, relay_port, room"""
        if self.running:
            return
        self.running = True
        self.mode = config.get("mode", "local")
        self._mss = mss.MSS()
        mon = self._mss.monitors[1]
        fps = max(config.get("fps", DEFAULT_FPS), 1)

        # Start pipeline threads
        self._raw_q = queue.Queue(maxsize=4)
        self._jpeg_q = queue.Queue(maxsize=4)

        t_cap = threading.Thread(target=self._capture_loop, args=(mon, fps), daemon=True)
        t_enc = threading.Thread(target=self._encode_loop, args=(config.get("quality", DEFAULT_Q),), daemon=True)
        t_send = threading.Thread(
            target=self._send_loop,
            args=(
                config.get("ip", DEFAULT_IP),
                config.get("port", DEFAULT_PORT),
                config.get("relay_host", RELAY_HOST_DEFAULT),
                config.get("relay_port", RELAY_PORT_DEFAULT),
                config.get("room", ROOM_DEFAULT),
            ),
            daemon=True,
        )
        t_cap.start()
        t_enc.start()
        t_send.start()
        self.thread = t_send

    def stop(self):
        self.running = False
        if self._loop and self._loop.is_running():
            self._loop.call_soon_threadsafe(lambda: None)
        if self.thread and self.thread.is_alive():
            self.thread.join(timeout=2)
        if self._mss:
            try: self._mss.close()
            except Exception: pass
            self._mss = None
        self.q.put(("disconnected", ""))


engine = StreamEngine()


# ── GUI ──────────────────────────────────────────────────────────────────────
root = Tk()
root.title("ESP32 Screen Mirror v3")
root.resizable(False, False)
root.configure(bg="#1e1e2e")

f = Frame(root, bg="#1e1e2e", padx=15, pady=15)
f.pack()

# ── Mode Selector ──
mode_var = StringVar(value="local")
mode_frame = Frame(f, bg="#1e1e2e")
mode_frame.grid(row=0, column=0, columnspan=3, sticky=W, pady=(0, 10))
Label(mode_frame, text="Mode:", fg="#cdd6f4", bg="#1e1e2e",
      font=("Segoe UI", 10)).pack(side="left")
rb_local = ttk.Radiobutton(mode_frame, text="Local (WiFi)", variable=mode_var,
                            value="local", command=lambda: toggle_remote_fields())
rb_remote = ttk.Radiobutton(mode_frame, text="Remote (Cloud Relay)", variable=mode_var,
                             value="remote", command=lambda: toggle_remote_fields())
rb_local.pack(side="left", padx=(8, 4))
rb_remote.pack(side="left", padx=4)

# ── Local fields (row 1-2) ──
lbl_ip = Label(f, text="ESP32 IP Address", fg="#cdd6f4", bg="#1e1e2e",
               font=("Segoe UI", 10))
lbl_ip.grid(row=1, column=0, sticky=W, pady=(0, 2))
ip_var = StringVar(value=DEFAULT_IP)
ip_entry = Entry(f, textvariable=ip_var, bg="#313244", fg="#cdd6f4",
                 insertbackground="#cdd6f4", font=("Segoe UI", 10),
                 relief="flat", width=16)
ip_entry.grid(row=2, column=0, sticky=W, pady=(0, 8))

lbl_port = Label(f, text="Port", fg="#cdd6f4", bg="#1e1e2e",
                 font=("Segoe UI", 10))
lbl_port.grid(row=1, column=1, sticky=W, padx=(10, 0), pady=(0, 2))
port_var = StringVar(value=str(DEFAULT_PORT))
port_entry = Entry(f, textvariable=port_var, bg="#313244", fg="#cdd6f4",
                   insertbackground="#cdd6f4", font=("Segoe UI", 10),
                   relief="flat", width=7)
port_entry.grid(row=2, column=1, sticky=W, padx=(10, 0), pady=(0, 8))

# ── Remote fields (row 1r-2r) ──
lbl_relay = Label(f, text="Relay Host", fg="#cdd6f4", bg="#1e1e2e",
                  font=("Segoe UI", 10))
lbl_relay.grid(row=3, column=0, sticky=W, pady=(0, 2))
relay_var = StringVar(value=RELAY_HOST_DEFAULT)
relay_entry = Entry(f, textvariable=relay_var, bg="#313244", fg="#cdd6f4",
                    insertbackground="#cdd6f4", font=("Segoe UI", 10),
                    relief="flat", width=22)
relay_entry.grid(row=4, column=0, columnspan=2, sticky=W, pady=(0, 2))

lbl_room = Label(f, text="Room Token", fg="#cdd6f4", bg="#1e1e2e",
                 font=("Segoe UI", 10))
lbl_room.grid(row=3, column=2, sticky=W, padx=(10, 0), pady=(0, 2))
room_var = StringVar(value=ROOM_DEFAULT)
room_entry = Entry(f, textvariable=room_var, bg="#313244", fg="#cdd6f4",
                   insertbackground="#cdd6f4", font=("Segoe UI", 10),
                   relief="flat", width=14)
room_entry.grid(row=4, column=2, sticky=W, padx=(10, 0), pady=(0, 2))

def toggle_remote_fields():
    is_local = mode_var.get() == "local"
    state = "normal" if is_local else "disabled"
    rstate = "disabled" if is_local else "normal"
    for w in [ip_entry, port_entry]: w.config(state=state)
    lbl_ip.config(fg="#cdd6f4" if is_local else "#585b70")
    lbl_port.config(fg="#cdd6f4" if is_local else "#585b70")
    for w in [relay_entry, room_entry]: w.config(state=rstate)
    lbl_relay.config(fg="#585b70" if is_local else "#cdd6f4")
    lbl_room.config(fg="#585b70" if is_local else "#cdd6f4")

toggle_remote_fields()

# ── JPEG Quality (row 5-6) ──
Label(f, text="JPEG Quality", fg="#cdd6f4", bg="#1e1e2e",
      font=("Segoe UI", 10)).grid(row=5, column=0, sticky=W, pady=(10, 2))
qv = StringVar(value=str(DEFAULT_Q))
qs = Scale(f, from_=25, to=95, orient=HORIZONTAL, variable=qv, length=180,
           bg="#313244", fg="#cdd6f4", troughcolor="#585b70",
           highlightthickness=0, font=("Segoe UI", 8),
           command=lambda v: qlv.set(f"{int(float(v))}"))
qs.grid(row=6, column=0, columnspan=2, sticky=W + E, pady=(0, 12))
qlv = StringVar(value=str(DEFAULT_Q))
Label(f, textvariable=qlv, fg="#a6e3a1", bg="#1e1e2e",
      font=("Segoe UI", 9, "bold")).grid(row=6, column=2, sticky=E, padx=(10, 0))

# ── Target FPS (row 7-8) ──
Label(f, text="Target FPS", fg="#cdd6f4", bg="#1e1e2e",
      font=("Segoe UI", 10)).grid(row=7, column=0, sticky=W)
fv = StringVar(value=str(DEFAULT_FPS))
fs = Scale(f, from_=5, to=60, orient=HORIZONTAL, variable=fv, length=180,
           bg="#313244", fg="#cdd6f4", troughcolor="#585b70",
           highlightthickness=0, font=("Segoe UI", 8),
           command=lambda v: flv.set(f"{int(float(v))}"))
fs.grid(row=8, column=0, columnspan=2, sticky=W + E, pady=(0, 12))
flv = StringVar(value=str(DEFAULT_FPS))
Label(f, textvariable=flv, fg="#a6e3a1", bg="#1e1e2e",
      font=("Segoe UI", 9, "bold")).grid(row=8, column=2, sticky=E, padx=(10, 0))

# ── Status + Start button ──
sv = StringVar(value="Ready")
sl = Label(f, textvariable=sv, fg="#f9e2af", bg="#1e1e2e",
           font=("Segoe UI", 9, "italic"))
sl.grid(row=9, column=0, pady=(5, 8), sticky=W)


def toggle():
    if engine.running:
        engine.stop()
        btn.config(text="Start Streaming", bg="#a6e3a1", fg="#1e1e2e")
        sv.set("Stopped"); sl.config(fg="#f38ba8")
    else:
        is_local = mode_var.get() == "local"
        config = {
            "mode": "local" if is_local else "remote",
            "ip": ip_var.get().strip() or DEFAULT_IP,
            "port": int(port_var.get().strip() or DEFAULT_PORT),
            "quality": int(qv.get() or DEFAULT_Q),
            "fps": int(fv.get() or DEFAULT_FPS),
            "relay_host": relay_var.get().strip() or RELAY_HOST_DEFAULT,
            "relay_port": RELAY_PORT_DEFAULT,
            "room": room_var.get().strip() or ROOM_DEFAULT,
        }
        engine.start(config)
        btn.config(text="Stop Streaming", bg="#f38ba8", fg="#1e1e2e")
        sv.set("Connecting..."); sl.config(fg="#f9e2af")


btn = Button(f, text="Start Streaming", command=toggle,
             bg="#a6e3a1", fg="#1e1e2e", font=("Segoe UI", 11, "bold"),
             relief="flat", padx=20, pady=6, cursor="hand2")
btn.grid(row=10, column=0, columnspan=3, sticky=W + E, pady=(8, 0))


def poll():
    while not engine.q.empty():
        k, v = engine.q.get_nowait()
        if k == "connected":
            sv.set("Connected"); sl.config(fg="#a6e3a1")
        elif k == "disconnected":
            sv.set("Disconnected"); sl.config(fg="#f38ba8")
        elif k == "fps":
            sv.set(f"FPS: {v:.1f}"); sl.config(fg="#89b4fa")
        elif k == "error":
            sv.set(f"Error: {v}"); sl.config(fg="#f38ba8")
    root.after(200, poll)


def on_close():
    engine.running = False
    if engine._loop and engine._loop.is_running():
        engine._loop.call_soon_threadsafe(lambda: None)
    if engine._mss:
        try: engine._mss.close()
        except Exception: pass
    root.destroy()


root.protocol("WM_DELETE_WINDOW", on_close)
atexit.register(lambda: setattr(engine, 'running', False))
root.after(500, poll)
root.mainloop()