// Simple data for the 3 zones
const zones = {
  1: { moisture: 30, watering: false, blocked: false },
  2: { moisture: 30, watering: false, blocked: false },
  3: { moisture: 30, watering: false, blocked: false }
};

const logList = document.getElementById("log-list");

function log(msg) {
  const item = document.createElement("li");
  item.textContent = `[${new Date().toLocaleTimeString()}] ${msg}`;
  logList.prepend(item);

  // Keep only the most recent 15 entries
  const MAX_LOG_ITEMS = 15;
  while (logList.children.length > MAX_LOG_ITEMS) {
    logList.removeChild(logList.lastChild);
  }
}


// Helper to get element by base id + zone number
function el(baseId, zone) {
  return document.getElementById(`${baseId}-${zone}`);
}

function updateLogic(zone) {
  const z = zones[zone];
  const moistureValue = el("moistureValue", zone);
  const statusText = el("status-text", zone);

  moistureValue.textContent = `Moisture: ${z.moisture}%`;

  if (z.moisture < 40) {
    z.blocked = false;
    z.watering = true;
    statusText.textContent = "Watering is ON";
    log(`Zone ${zone}: auto -> soil dry (${z.moisture}%) -> watering ON.`);
  } else if (z.moisture <= 80) {
    z.blocked = false;
    z.watering = false;
    statusText.textContent = "Watering is OFF";
    log(`Zone ${zone}: auto -> soil healthy (${z.moisture}%) -> watering OFF.`);
  } else {
    z.blocked = true;
    z.watering = false;
    statusText.textContent = "Watering is OFF (Oversaturated)";
    log(`Zone ${zone}: auto -> soil oversaturated (${z.moisture}%) -> watering blocked.`);
  }
}

// Set up sliders + buttons for all zones
[1, 2, 3].forEach(zone => {
  const slider = el("moistureSlider", zone);
  const startBtn = el("startBtn", zone);
  const stopBtn  = el("stopBtn", zone);

  // Slider changes moisture and re-runs auto logic
  slider.addEventListener("input", e => {
    zones[zone].moisture = parseInt(e.target.value);
    updateLogic(zone);
  });

  // Manual start – allowed unless oversaturated
  startBtn.addEventListener("click", () => {
    const z = zones[zone];
    if (z.blocked) {
      log(`Zone ${zone}: manual override blocked (oversaturated).`);
      return;
    }
    z.watering = true;
    el("status-text", zone).textContent = "Watering is ON (manual override)";
    log(`Zone ${zone}: manual watering ON.`);
  });

  // Manual stop – always allowed
  stopBtn.addEventListener("click", () => {
    zones[zone].watering = false;
    el("status-text", zone).textContent = "Watering is OFF";
    log(`Zone ${zone}: manual watering OFF.`);
  });

  // Initial render
  updateLogic(zone);
});
