#!/usr/bin/env python3
"""
ESP32 Mirror Relay - Room-based WebSocket frame forwarder
Pairs one sender (GUI) to one display (ESP32) per room token.
Deploy: Render.com Web Service (free tier) - Python 3.11+
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

# Config
PORT = int(os.environ.get("PORT", 8000))
HOST = "0.0.0.0"
DEFAULT_ROOM = os.environ.get("ROOM_TOKEN", "deniz-mirror").strip().lower()
WS_MAX_SIZE = int(os.environ.get("WS_MAX_SIZE", 4 * 1024 * 1024))

rooms = {}


async def health_check(connection, request):
    """HTTP health endpoint. Must never raise - crash = 502 for everyone."""
    try:
        path = request.path if request.path else "/"
        if path == "/":
            headers = request.headers or {}
            upgrade = str(headers.get("upgrade", "")).lower()
            if "websocket" not in upgrade:
                try:
                    await connection.respond(200, "ESP32 Mirror Relay - OK")
                except Exception:
                    pass
                return True
    except Exception:
        pass
    return False


def get_room(ws) -> str:
    """Extract room token from WebSocket query string."""
    try:
        raw_q = getattr(ws.request, "raw_query", None) or getattr(ws.request, "query", None) or ""
    except AttributeError:
        raw_q = ""
    if raw_q:
        qs = parse_qs(raw_q)
        room = qs.get("room", [None])[0]
        if room:
            return room.strip().lower()
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


async def handle(ws):
    """Assign sender/display role, relay binary frames."""
    room_id = get_room(ws)
    if room_id not in rooms:
        rooms[room_id] = {"sender": None, "display": None}
    room = rooms[room_id]
    peer = ws.remote_address[0] if ws.remote_address else "?"

    # Assign role
    role = "display"
    if room["display"] is None and room["sender"] is not None:
        role = "display"
    elif room["display"] is not None and room["sender"] is None:
        role = "sender"
    elif room["display"] is None and room["sender"] is None:
        role = "display"
    else:
        role = "display"

    old = room.get(role)
    if old:
        try:
            await old.close(1001, "replaced")
        except Exception:
            pass

    room[role] = ws
    log.info("[%s] %s connected (%s)", room_id, role.upper(), peer)

    try:
        async for message in ws:
            if role == "sender" and isinstance(message, bytes):
                target = room.get("display")
                if target:
                    try:
                        await target.send(message)
                    except Exception:
                        room["display"] = None
                        log.info("[%s] display disconnected (send error)", room_id)
    except websockets.exceptions.ConnectionClosed:
        pass
    except Exception as e:
        log.warning("[%s] %s error: %s", room_id, role, e)

    if room.get(role) is ws:
        room[role] = None
    log.info("[%s] %s disconnected", room_id, role.upper())

    if room["sender"] is None and room["display"] is None:
        rooms.pop(room_id, None)
        log.info("[%s] room deleted", room_id)


async def prune_task():
    while True:
        await asyncio.sleep(60)
        dead = []
        for rid, r in list(rooms.items()):
            sender = r.get("sender")
            if sender and not r.get("display"):
                try:
                    await sender.close(1001, "no display")
                except Exception:
                    pass
                r["sender"] = None
            if not r.get("sender") and not r.get("display"):
                dead.append(rid)
        for rid in dead:
            rooms.pop(rid, None)
            log.info("[%s] pruned", rid)


async def main():
    log.info("Mirror Relay v3 on %s:%s (max %s MB)", HOST, PORT, WS_MAX_SIZE // 1024 // 1024)
    async with serve(
        handle, HOST, PORT,
        max_size=WS_MAX_SIZE,
        process_request=health_check,
        ping_interval=20,
        ping_timeout=10,
        close_timeout=10,
    ):
        await prune_task()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        log.info("Shutdown")