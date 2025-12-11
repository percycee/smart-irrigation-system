// ===== DOM refs =====
const moistureValueEl  = document.getElementById("moistureValue");
const sensorValueEl    = document.getElementById("sensorValue");
const moistureLabelEl  = document.getElementById("moistureLabel");
const statusTextEl     = document.getElementById("status-text");
const alertBoxEl       = document.getElementById("alert-box");
const logListEl        = document.getElementById("log-list");

const startBtn         = document.getElementById("startBtn");
const stopBtn          = document.getElementById("stopBtn");

// keep log short
const MAX_LOG_ITEMS = 40;

// ===== helpers =====
function log(msg) {
  const li = document.createElement("li");
  li.textContent = `[${new Date().toLocaleTimeString()}] ${msg}`;
  logListEl.appendChild(li);

  // scroll to bottom
  logListEl.scrollTop = logListEl.scrollHeight;

  // trim
  while (logListEl.children.length > MAX_LOG_ITEMS) {
    logListEl.removeChild(logListEl.firstChild);
  }
}

function setAlert(msg) {
  if (!msg) {
    alertBoxEl.style.display = "none";
    alertBoxEl.textContent = "";
  } else {
    alertBoxEl.style.display = "block";
    alertBoxEl.textContent = msg;
  }
}

// 0–100% -> label string
function classifyMoisture(pct) {
  if (isNaN(pct)) return "--";
  if (pct < 40)  return "Too dry";
  if (pct <= 80) return "Healthy";
  return "Oversaturated";
}

function setStatusChip(percent, isWatering) {
  statusTextEl.classList.remove("status-on", "status-off", "status-blocked");

  const blocked = percent > 80;

  if (blocked) {
    statusTextEl.classList.add("status-blocked");
    statusTextEl.textContent = "Watering blocked (oversaturated)";
  } else if (isWatering) {
    statusTextEl.classList.add("status-on");
    statusTextEl.textContent = "Watering is ON";
  } else {
    statusTextEl.classList.add("status-off");
    statusTextEl.textContent = "Watering is OFF";
  }
}

// ===== backend polling =====
let hadBackendError = false;

async function fetchStatus() {
  try {
    const resp = await fetch("/api/status");
    if (!resp.ok) throw new Error("HTTP " + resp.status);
    const data = await resp.json();

    // raw reading from ESP32
    const raw = Number(data.moisture);
    const watering = Number(data.isWatering) === 1;

    if (!Number.isFinite(raw)) return;

    // convert 0–4095 -> 0–100%
    let pct = Math.round((raw / 4095) * 100);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;

    // update UI numbers
    moistureValueEl.textContent = `${pct} %`;
    sensorValueEl.textContent   = `Sensor: ${raw}`;
    moistureLabelEl.textContent = `Status: ${classifyMoisture(pct)}`;

    setStatusChip(pct, watering);

    if (hadBackendError) {
      log("Reconnected to backend.");
    }
    hadBackendError = false;
    setAlert("");

  } catch (err) {
    if (!hadBackendError) {
      log("Error talking to backend.");
    }
    hadBackendError = true;
    setAlert("Error talking to backend.");
  }
}

setInterval(fetchStatus, 2000); // poll every 2 seconds

// ===== buttons =====
startBtn.addEventListener("click", async () => {
  log("Start button pressed (requesting manual watering from ESP32).");
  try {
    const resp = await fetch("/api/water/start", { method: "POST" });
    if (!resp.ok) throw new Error("HTTP " + resp.status);
    const txt = await resp.text();
    // backend just returns "ok" right now
    setAlert("");
  } catch (err) {
    log("Failed to send manual watering request.");
    setAlert("Failed to send manual watering request.");
  }
});

stopBtn.addEventListener("click", () => {
  // In current firmware, watering stops automatically after its timer.
  // We just log that the user pressed stop for the demo.
  log("Stop button pressed (ESP32 will stop when its timer expires).");
});

// ===== initial =====
log("UI loaded. Zone 1 is live hardware data.");
fetchStatus();
