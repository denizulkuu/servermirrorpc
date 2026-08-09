#!/usr/bin/env python3
"""
ESP32 Mirror Relay - Room-based WebSocket frame forwarder.
Pairs one sender (GUI) to one display (ESP32) per room token.
Deploy: Render.com Web Service (free tier) - Python 3.11+
"""

import asyncio, os, sys, logging
from urllib.parse import parse_qs

try:
    import websockets
    from websockets.asyncio.server import serve
except ImportError:
    print("pip install websockets>=12.0"); sys.exit(1)

logging.basicConfig(level=logging.INFO,
    format="%(asctime)s  %(name)-10s %(levelname)-5s %(message)s")
log = logging.getLogger("relay")

PORT = int(os.environ.get("PORT", 8000))
WS_MAX_SIZE = int(os.environ.get("WS_MAX_SIZE", 4 * 1024 * 1024))
rooms = {}


def get_room(ws) -> str:
    """Extract room token from WebSocket query string."""
    # websockets v17: query is on ws.request
    try:
        raw = (getattr(ws.request, "raw_query", None)
               or getattr(ws.request, "query", None) or "")
    except AttributeError:
        raw = ""
    if raw:
        qs = parse_qs(raw)
        room = qs.get("room", [None])[0]
        if room:
            return room.strip().lower()
    # Fallback for older versions
    try:
        from urllib.parse import urlparse
        qs = parse_qs(urlparse(ws.request.path).query)
        room = qs.get("room", [None])[0]
        if room:
            return room.strip().lower()
    except Exception:
        pass
    return "default"


async def process_request(connection, request):
    """
    websockets v17 process_request handler.
    Return (status, headers, body) to respond with HTTP.
    Return None to proceed with WebSocket upgrade.
    """
    # If the client wants to upgrade to WebSocket, let it through
    headers = request.headers or {}
    upgrade = str(headers.get("upgrade", "")).lower()
    if "websocket" in upgrade:
        return None  # proceed with WS handshake

    # Otherwise, serve a health-check HTTP response
    return (200, [("Content-Type", "text/plain")],
            "ESP32 Mirror Relay - OK")


async def handle(ws):
    """Assign sender/display role, relay binary frames."""
    room_id = get_room(ws)
    room = rooms.setdefault(room_id, {"sender": None, "display": None})
    peer = ws.remote_address[0] if ws.remote_address else "?"

    # Role assignment: empty room -> display first, then sender
    if room["display"] is None:
        role = "display"
    elif room["sender"] is None:
        role = "sender"
    else:
        role = "display"  # ESP32 reconnects more often, replace display

    # Kick old occupant of same role
    old = room.get(role)
    if old:
        try: await old.close(1001, "replaced")
        except Exception: pass

    room[role] = ws
    log.info("[%s] %s connected (%s)", room_id, role.upper(), peer)

    # Relay loop
    try:
        async for message in ws:
            if role == "sender" and isinstance(message, bytes):
                target = room.get("display")
                if target:
                    try: await target.send(message)
                    except Exception:
                        room["display"] = None
                        log.info("[%s] display dropped", room_id)
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
        for rid, r in list(rooms.items()):
            if r["sender"] and not r["display"]:
                try: await r["sender"].close(1001, "no display")
                except Exception: pass
                r["sender"] = None
            if not r["sender"] and not r["display"]:
                rooms.pop(rid, None)
                log.info("[%s] pruned", rid)


async def main():
    log.info("Mirror Relay v4 on 0.0.0.0:%d (%d MB)", PORT, WS_MAX_SIZE // 1024 // 1024)
    async with serve(handle, "0.0.0.0", PORT,
                     max_size=WS_MAX_SIZE,
                     process_request=process_request):
        await prune_task()


if __name__ == "__main__":
    try: asyncio.run(main())
    except KeyboardInterrupt: log.info("Shutdown")