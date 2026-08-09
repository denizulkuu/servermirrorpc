#!/usr/bin/env python3
"""
ESP32 Mirror Relay — Room-based WebSocket frame forwarder
Pairs one sender (GUI) to one display (ESP32) per room token.

Deploy: Render.com Web Service (free tier, 750 hrs/month)
  Runtime:  Python 3.11+
  Build:    pip install -r requirements_server.txt
  Start:    python relay_server.py
  Health:   Render pings GET / → 200 OK (keeps service alive)
"""

import asyncio
import os
import sys
import logging
from urllib.parse import parse_qs

try:
    import websockets
    from websockets.asyncio.server import serve
except ImportError:
    print("pip install websockets>=12.0")
    sys.exit(1)

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s  %(name)-12s %(levelname)-5s %(message)s"
)
log = logging.getLogger("relay")

# ── Config ────────────────────────────────────────────────────────────────────
PORT = int(os.environ.get("PORT", 8000))
HOST = "0.0.0.0"

# room_id → {"sender": ws | None, "display": ws | None}
rooms = {}


# ── Health check handler (keeps Render.com free tier alive) ───────────────────
async def health_check(connection, request):
    """HTTP handler: Render pings this every few minutes → returns 200."""
    # Check User-Agent to distinguish HTTP probes from WS upgrades
    if connection.state.name != "CONNECTING":
        return
    if request.path == "/" and "websocket" not in str(request.headers.get("upgrade", "")).lower():
        # This is an HTTP GET, not a WebSocket upgrade — serve health page
        html = b"""<!DOCTYPE html>
<html><head><title>ESP32 Mirror Relay</title></head>
<body style="font-family:sans-serif;text-align:center;padding-top:80px;background:#1e1e2e;color:#cdd6f4">
<h1 style="color:#a6e3a1">ESP32 Mirror Relay</h1>
<p>Online. Connect with room token: <code>/?room=YOUR_TOKEN</code></p>
<p style="color:#89b4fa;font-size:12px">WebSocket only - no web UI here.</p>
</body></html>"""
        connection.respond(200, html)
        return True  # handled
    return False  # not our HTTP, let WS upgrade proceed


# ── Room extraction (reads actual query string) ────────────────────────────────
def get_room(ws) -> str:
    """
    Extract room token from WebSocket request query string.
    websockets v12+: ws.request.path = "/" (no query), query is at ws.request.query
    Falls back to URL parsing for older versions.
    """
    # Primary: decoded query string (websockets v12+)
    try:
        raw_q = getattr(ws.request, "raw_query", None) or ws.request.query or ""
    except AttributeError:
        raw_q = ""
    
    if raw_q:
        qs = parse_qs(raw_q)
        room = qs.get("room", [None])[0]
        if room:
            return room.strip().lower()
    
    # Fallback: parse path + query the old way (works on websockets v10-v11)
    try:
        from urllib.parse import urlparse
        parsed = urlparse(ws.request.path)
        qs = parse_qs(parsed.query)
        room = qs.get("room", [None])[0]
        if room:
            return room.strip().lower()
    except Exception:
        pass
    
    return "default"


# ── Connection handler ─────────────────────────────────────────────────────────
async def handle(ws):
    """Each new WebSocket connection gets assigned as sender or display."""
    room_id = get_room(ws)

    if room_id not in rooms:
        rooms[room_id] = {"sender": None, "display": None}

    room = rooms[room_id]
    peer = ws.remote_address[0] if ws.remote_address else "?"

    # ── Assign role ──────────────────────────────────────────────────────────
    # Strategy: first connection → display (ESP32), second → sender (GUI)
    # If ESP32 reconnects, replace old display. If GUI reconnects, replace old sender.
    role = "display"
    if room["display"] is None and room["sender"] is not None:
        # Only sender exists → this must be display
        role = "display"
    elif room["display"] is not None and room["sender"] is None:
        # Only display exists → this must be sender
        role = "sender"
    elif room["display"] is None and room["sender"] is None:
        # Empty room → first connection is display
        role = "display"
    else:
        # Both exist — replace whichever reconnects
        # Check which side is stale by trying a ping
        role = "display"  # default: assume ESP32 reconnects more often

    # Replace existing connection of same role
    old = room.get(role)
    if old:
        try:
            await old.close(1001, "replaced")
        except Exception:
            pass

    room[role] = ws
    log.info(f"[{room_id}] {role.upper():7s} connected  ({peer})")

    # ── Relay loop ───────────────────────────────────────────────────────────
    try:
        async for message in ws:
            if role == "sender" and isinstance(message, bytes):
                # Forward binary frames from GUI → ESP32
                target = room.get("display")
                if target:
                    try:
                        await target.send(message)
                    except websockets.exceptions.ConnectionClosed:
                        room["display"] = None
                        log.info(f"[{room_id}] DISPLAY disconnected (send error)")
            # Display messages are ignored (ESP32 sends nothing useful)
    except websockets.exceptions.ConnectionClosed:
        pass
    except Exception as e:
        log.warning(f"[{room_id}] {role} error: {type(e).__name__}: {e}")

    # ── Cleanup ──────────────────────────────────────────────────────────────
    if room.get(role) is ws:
        room[role] = None
    log.info(f"[{room_id}] {role.upper():7s} disconnected")

    # Remove empty rooms
    if room["sender"] is None and room["display"] is None:
        rooms.pop(room_id, None)
        log.info(f"[{room_id}] room deleted (empty)")


# ── Background: prune dead rooms every 60 seconds ─────────────────────────────
async def prune_task():
    while True:
        await asyncio.sleep(60)
        dead = []
        for rid, r in list(rooms.items()):
            # Close senders that have no display paired
            sender = r.get("sender")
            if sender and not r.get("display"):
                try:
                    await sender.close(1001, "no display paired")
                except Exception:
                    pass
                r["sender"] = None
            # Track empty rooms
            if not r.get("sender") and not r.get("display"):
                dead.append(rid)
        for rid in dead:
            rooms.pop(rid, None)
            log.info(f"[{rid}] room pruned (idle)")

        if rooms:
            status = " | ".join(
                f"{rid}: S={int(r['sender'] is not None)} D={int(r['display'] is not None)}"
                for rid, r in sorted(rooms.items())
            )
            log.debug(f"Active rooms: {status or 'none'}")


# ── Ping Render.com itself to prevent cold starts ─────────────────────────────
async def self_ping_task():
    """External pinger — useless since we ARE the server. Skip."""
    pass  # Render.com pings us via the health_check handler above


# ── Main ───────────────────────────────────────────────────────────────────────
async def main():
    log.info(f"✦ Mirror Relay v2 starting on {HOST}:{PORT}")
    log.info(f"  Health:  GET http://host:{PORT}/ → 200 OK")
    log.info(f"  Connect: ws://host:{PORT}/?room=TOKEN")
    log.info(f"  Max frame size: 4 MB")
    log.info(f"  python-websockets: {websockets.__version__}")

    async with serve(
        handle,
        HOST,
        PORT,
        max_size=4 * 1024 * 1024,
        process_request=health_check,   # HTTP health check handler
        ping_interval=20,                # keepalive WebSocket pings
        ping_timeout=10,
        close_timeout=10,
    ):
        # Run prune task concurrently with the server
        await prune_task()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        log.info("Shutdown by user")