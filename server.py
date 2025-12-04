import asyncio
from typing import List, Optional
from aiohttp import web, ClientSession
import os
import json
import jinja2
import aiohttp
import aiohttp_jinja2
from datetime import datetime

LOG_FILE = 'log.json'
log = []

def load_log():
  global log
  if os.path.exists(LOG_FILE):
    with open(LOG_FILE, 'r') as f:
      try:
        log = json.load(f)  
      except json.JSONDecodeError:
        log = []

def append_log(log_entry):
    with open(LOG_FILE, 'a') as f:
        json.dump([log_entry], f)  # Write the log entry in JSON format
        f.write('\n')

def add_log(event_type: str, message: str):
    log_entry = {
        "timestamp": datetime.now().isoformat(),
        "event_type": event_type,
        "message": message
    }
    log.append(log_entry)
    append_log(log_entry)

async def sse_handler(request):
    # Setting up SSE response headers
    response = web.StreamResponse(status=200)
    response.content_type = 'text/event-stream'
    response.headers['Cache-Control'] = 'no-cache'
    response.headers['Connection'] = 'keep-alive'
    await response.prepare(request)
    
    try:
        while True:
            if log:
                last_log = log[-1]
                # Send the last log entry as SSE
                await response.write(f"data: {json.dumps(last_log)}\n\n".encode())
            await asyncio.sleep(1)  
    except asyncio.CancelledError:
        pass
    return response

def app():
  routes = web.RouteTableDef()

  @routes.get('/')
  async def index(request):
    context = {
            "log": json.dumps(log, indent=2)
        }
    response = aiohttp_jinja2.render_template("index.html", request, context)
    return response

  @routes.post('/manual-override')
  async def manual_override(request):   
    form = await request.post()
    timestamp = form.get("timestamp")  # Get the timestamp sent with the button
    if timestamp:
      add_log("manual_override", f"Manual override triggered at {timestamp}")
    else:
      add_log("manual_override", "Manual override triggered without timestamp")
    return web.Response(text="Manual override triggered")

  @routes.post('/adjust-settings')
  async def adjust_settings(request):
    form = await request.post()
    slider_value = form.get('slider_value')
    if slider_value is None:
      return web.Response(text="Slider value is missing", status=400)
    add_log("adjust_settings", f"Slider adjusted to {slider_value}")
    return web.Response(text=f"Slider adjusted to {slider_value}")

  @routes.post('/moisture-data')
  async def moisture_data(request):
    data = await request.json()
    sensor_value = data.get('sensor_value')
    if sensor_value is None:
      return web.Response(text="Sensor value is missing", status=400)
    add_log("sensor_data", f"Received sensor value: {sensor_value}")
    return web.Response(text="Moisture received successfully")


  app = web.Application()
  app.add_routes(routes)
  aiohttp_jinja2.setup(app,
                       loader=jinja2.FileSystemLoader('templates'))  

  return app


def main():
  web.run_app(app(), port=int(os.environ.get("PORT", "8080")))


if __name__ == "__main__":
  main()
