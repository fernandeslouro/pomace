# Pi Web App (Pomace Boiler)

This folder contains a Raspberry Pi web app for local boiler monitoring/control.

## What it does
- Exposes HTTP API to the boiler controller over serial (`/dev/ttyUSB0` by default)
- Serves a small dashboard UI (status + control buttons)
- Shows control authority (`FULL` only when effective mode is `REMOTE`)
- Supports overrides already in firmware:
  - `FLAME ON/AUTO`
  - `STAGE 0/1/2/3/AUTO`
  - `FAN <AUTO|0..100>`
  - `SET <MOTOR> <ON|OFF|AUTO>`
  - `JAM <MOTOR>`

## Local run
```bash
cd pi-webapp
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
cp .env.example .env
./run.sh
```

Open: `http://<pi-ip>:8080`

## API endpoints
- `GET /api/health`
- `GET /api/status`
- `POST /api/mode` body: `{"mode":"AUTO|OFF|REMOTE"}`
- `POST /api/thermostat` body: `{"state":"AUTO|0|1"}`
- `POST /api/flame` body: `{"state":"AUTO|ON"}`
- `POST /api/stage` body: `{"stage":"AUTO|0|1|2|3"}`
- `POST /api/fan` body: `{"value":"AUTO|0..100"}`
- `POST /api/motor` body: `{"motor":"SF|PH|PWH|SB|FSG|FC|STORAGE|CRUSHER","state":"AUTO|ON|OFF"}`
- `POST /api/jam` body: `{"motor":"SF|PH|PWH|SB|FSG|FC|STORAGE|CRUSHER"}`
- `POST /api/reset` body: `{"target":"BOILER|SAFETY|ALL|<MOTOR>"}`
- `POST /api/raw` body: `{"command":"STATUS"}`

## Control authority model
- The panel selector defines effective mode.
- Full app control requires board mode to be `REMOTE`.
- In `AUTO`/`OFF`, app commands are accepted but behavior is constrained by panel mode.

## Network strategy

### Option A: same LAN/Wi-Fi
- Pi and phone/laptop on same network
- Access by Pi LAN IP (best if Pi has stable Ethernet uplink)

### Option B: Tailscale (recommended if Wi-Fi is weak)
- Install Tailscale on Pi and client devices
- Bring the Pi online in tailnet
- Access with Tailscale IP or MagicDNS host over port `8080`

A practical setup is:
- Pi to router via Ethernet (if available)
- Operator devices over Wi-Fi or cellular + Tailscale

## systemd service
Adjust paths/user in `systemd/pomace-webapp.service`, then:

```bash
sudo cp systemd/pomace-webapp.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now pomace-webapp
sudo systemctl status pomace-webapp
```
