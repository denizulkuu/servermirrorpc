#!/usr/bin/env python3
"""ESP32 Mirror Relay - Room-based WebSocket frame forwarder."""

import asyncio, os, sys, logging
from urllib.parse import parse_qs, urlparse

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

# Store query params during handshake, match in handle()
# Key: ws object itself (same request object across process_request and handle)
query_store = {}


async def process_request(connection, request):
    """Intercept HTTP→WS upgrade. Save query params, serve health check."""
    # Parse query string from raw request path (available here, not later)
    raw_url = request.path or "/"
    if "?" in raw_url:
        q_part = raw_url.split("?", 1)[1]
        qs = parse_qs(q_part)
        query_store[id(connection)] = {
            "room": (qs.get("room", ["default"])[0] or "default").strip().lower(),
            "role": (qs.get("role", [None])[0] or "").strip().lower(),
        }
    else:
        query_store[id(connection)] = {"room": "default", "role": ""}

    # Health check for non-WebSocket requests
    headers = request.headers or {}
    upgrade = str(headers.get("upgrade", "")).lower()
    if "websocket" not in upgrade:
        return connection.respond(200, "ESP32 Mirror Relay - OK")
    return None  # proceed with WS upgrade


async def handle(ws):
    """Assign sender/display role, relay binary frames."""
    cid = id(ws)
    qinfo = query_store.pop(cid, {"room": "default", "role": ""})
    room_id = qinfo["room"]
    role_req = qinfo["role"]

    room = rooms.setdefault(room_id, {"sender": None, "display": None})
    peer = ws.remote_address[0] if ws.remote_address else "?"

    # Role assignment
    if role_req in ("sender", "display"):
        role = role_req
    elif room["display"] is None:
        role = "display"
    elif room["sender"] is None:
        role = "sender"
    else:
        role = "display"

    # Replace existing connection of same role
    old = room.get(role)
    if old:
        try: await old.close(1001, "replaced")
        except Exception: pass

    room[role] = ws
    log.info("[%s] %s connected (%s)", room_id, role.upper(), peer)

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
        log.warning("[%s] %s error: %s", room_id, role, type(e).__name__)

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
    log.info("Mirror Relay v7 on 0.0.0.0:%d (%d MB)", PORT, WS_MAX_SIZE // 1024 // 1024)
    async with serve(handle, "0.0.0.0", PORT,
                     max_size=WS_MAX_SIZE,
                     process_request=process_request):
        await prune_task()


if __name__ == "__main__":
    try: asyncio.run(main())
    except KeyboardInterrupt: log.info("Shutdown")