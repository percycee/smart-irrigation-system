import os
import json
from datetime import datetime

from aiohttp import web, ClientSession
import aiohttp_jinja2
import jinja2

# =========================
# Config
# =========================

# CHANGE THIS to whatever IP the ESP32 prints in Serial Monitor.
# Example: "http://192.168.1.184"
ESP_IP = os.environ.get("ESP_IP", "http://192.168.68.56")

LOG_FILE = "log.json"
log = []

# =========================
# Simple logging to a file
# =========================

def load_log():
    """Load existing log.json if present."""
    global log
    if os.path.exists(LOG_FILE):
        try:
            with open(LOG_FILE, "r") as f:
                log = json.load(f)
        except (json.JSONDecodeError, OSError):
            log = []
    else:
        log = []

def append_log(entry):
    """Append a single log entry to log.json (newline-separated JSON)."""
    with open(LOG_FILE, "a") as f:
        json.dump([entry], f)
        f.write("\n")

def add_log(event_type: str, message: str):
    entry = {
        "timestamp": datetime.now().isoformat(),
        "event_type": event_type,
        "message": message,
    }
    log.append(entry)
    append_log(entry)

# =========================
# Web app
# =========================

def create_app():
    load_log()

    routes = web.RouteTableDef()
    app = web.Application()

    # ---------- API that talks to ESP32 ----------

    @routes.get("/api/status")
    async def api_status(request):
        """
        Proxy to ESP32:
          GET ESP_IP + /status
        Expected JSON from ESP32 (example):
          {"moisture": 350, "isWatering": 1}
        """
        async with ClientSession() as session:
            async with session.get(f"{ESP_IP}/status") as resp:
                text = await resp.text()
                # optional: you can parse & log it if you want
                return web.Response(text=text,
                                    content_type="application/json")

    @routes.post("/api/water/start")
    async def api_water_start(request):
        """
        Proxy to ESP32:
          POST ESP_IP + /water/start
        This triggers the 10-second watering on the board.
        """
        async with ClientSession() as session:
            async with session.post(f"{ESP_IP}/water/start") as resp:
                text = await resp.text()
                add_log("manual_override", "Manual watering requested from UI")
                return web.Response(text=text)

    # ---------- UI ----------

    @routes.get("/")
    async def index(request):
        # You can pass log if you want; UI doesn’t *need* it right now.
        context = {"log_json": json.dumps(log, indent=2)}
        return aiohttp_jinja2.render_template("user_interface.html",
                                              request,
                                              context)

    app.add_routes(routes)

    # Jinja2 templates in ./temp
    aiohttp_jinja2.setup(
        app,
        loader=jinja2.FileSystemLoader("temp")
    )

    # Static files (JS/CSS) in ./static
    app.router.add_static("/static/",
                          path=os.path.join(os.getcwd(), "static"),
                          name="static")

    return app


def main():
    web.run_app(create_app(), port=int(os.environ.get("PORT", "8080")))


if __name__ == "__main__":
    main()
