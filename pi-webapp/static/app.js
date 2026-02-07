const summaryGrid = document.getElementById("summary-grid");
const statusTs = document.getElementById("status-ts");
const motorBody = document.getElementById("motor-body");
const commandOutput = document.getElementById("command-output");
const authorityLine = document.getElementById("authority-line");

const summaryOrder = [
  "mode",
  "safety",
  "phase",
  "bflt",
  "b_lock",
  "retry",
  "rrestart",
  "temp_ok",
  "b",
  "hw",
  "wg",
  "fan",
  "flame",
  "fl_ovr",
  "stg",
  "astg",
  "faults",
];

function renderSummary(summary) {
  summaryGrid.innerHTML = "";
  for (const key of summaryOrder) {
    if (!(key in summary)) {
      continue;
    }
    const card = document.createElement("div");
    card.className = "stat";
    card.innerHTML = `<div class="k">${key}</div><div class="v">${summary[key]}</div>`;
    summaryGrid.appendChild(card);
  }
}

function renderMotors(motors) {
  motorBody.innerHTML = "";
  for (const motor of motors) {
    const row = document.createElement("tr");
    row.innerHTML = `
      <td>${motor.name}</td>
      <td>${motor.on ? "ON" : "OFF"}</td>
      <td>${motor.fault ? "1" : "0"}</td>
      <td>${motor.recovery ? "1" : "0"}</td>
    `;
    motorBody.appendChild(row);
  }
}

function renderAuthority(authority) {
  if (!authority || typeof authority !== "object") {
    authorityLine.textContent = "Control authority: unknown";
    return;
  }

  const full = authority.app_full_control === true;
  const mode = authority.effective_mode || "UNKNOWN";
  authorityLine.textContent = full
    ? `Control authority: FULL (mode=${mode})`
    : `Control authority: LIMITED by panel selector (mode=${mode})`;
}

async function api(path, method = "GET", body = null) {
  const response = await fetch(path, {
    method,
    headers: { "Content-Type": "application/json" },
    body: body ? JSON.stringify(body) : null,
  });

  const data = await response.json();
  if (!response.ok) {
    throw new Error(data.detail || "Request failed");
  }
  return data;
}

async function refreshStatus() {
  try {
    const data = await api("/api/status");
    renderSummary(data.summary || {});
    renderMotors(data.motors || []);
    renderAuthority(data.authority || null);
    statusTs.textContent = `Last update: ${new Date().toLocaleTimeString()}`;
  } catch (error) {
    statusTs.textContent = `Status error: ${error.message}`;
  }
}

async function sendCommand(action, value) {
  try {
    if (action === "mode") {
      await api("/api/mode", "POST", { mode: value });
    } else if (action === "thermostat") {
      await api("/api/thermostat", "POST", { state: value });
    } else if (action === "flame") {
      await api("/api/flame", "POST", { state: value });
    } else if (action === "stage") {
      await api("/api/stage", "POST", { stage: value });
    } else if (action === "reset") {
      await api("/api/reset", "POST", { target: value });
    }
    await refreshStatus();
  } catch (error) {
    commandOutput.textContent = `Command error: ${error.message}`;
  }
}

for (const button of document.querySelectorAll("button[data-action]")) {
  button.addEventListener("click", async () => {
    await sendCommand(button.dataset.action, button.dataset.value);
  });
}

document.getElementById("raw-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  const input = document.getElementById("raw-input");
  const command = input.value.trim();
  if (!command) {
    return;
  }

  try {
    const result = await api("/api/raw", "POST", { command });
    commandOutput.textContent = result.lines.join("\n");
  } catch (error) {
    commandOutput.textContent = `Raw command error: ${error.message}`;
  }

  await refreshStatus();
});

document.getElementById("motor-apply").addEventListener("click", async () => {
  const motor = document.getElementById("motor-select").value;
  const state = document.getElementById("motor-state").value;
  try {
    await api("/api/motor", "POST", { motor, state });
    await refreshStatus();
  } catch (error) {
    commandOutput.textContent = `Motor command error: ${error.message}`;
  }
});

document.getElementById("motor-jam").addEventListener("click", async () => {
  const motor = document.getElementById("motor-select").value;
  try {
    await api("/api/jam", "POST", { motor });
    await refreshStatus();
  } catch (error) {
    commandOutput.textContent = `JAM command error: ${error.message}`;
  }
});

refreshStatus();
setInterval(refreshStatus, 2000);
