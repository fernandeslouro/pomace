# Pomace Boiler Controller (NPBC-V3M Replacement)

## Project
Replace the existing Naturela NPBC-V3M boiler controller with a programmable controller + web app, while reusing panel hardware where possible.

## Current Decisions
- Country: Portugal (`230 VAC`, `50 Hz`)
- Reuse existing contactors
- Reuse existing sensors
- Keep analog fan output path
- Do not switch motor loads directly from controller outputs; use contactor/interposing stage
- Door switch not used
- Anti-jam applies to all motors except `FM`
- Budget target: around `200 EUR` for control hardware (web host separate)

## Repository Status
- Firmware implements:
- `AUTO/OFF/REMOTE` modes
- startup state machine (`purge -> feed -> flame prove -> run`)
- flame-loss restart policy and boiler process lockout
- flame override (`FLAME ON`) for failed/noisy flame sensor scenarios
- run stage override (`STAGE 0|1|2|3`) with return to automatic (`STAGE AUTO`)
- anti-jam/stall recovery framework for non-fan motors
- serial command interface (including `RESET BOILER`)
- fan override command (`FAN <AUTO|0..100>`) for REMOTE mode commissioning
- Key files:
- `algorithm.ino`
- `support.ino`
- `control_config.h` (timing/threshold tunables)
- `hardware_profile.h` (pin map and profile toggles)
- `pi-webapp/` (Raspberry Pi API + dashboard for LAN/Tailscale access)
- `NPBC_V3M_REPLACEMENT_BOM.md` (shopping list and wiring notes)
- `connection.png` (existing V3M wiring diagram reference)

## Recommended Control Hardware
- 1x Waveshare `ESP32-S3-ETH-8DI-8RO` (RS485 version, not `-C`)
- 1x RS485 Modbus analog output module (`0-10V` voltage-output version) for fan command
- 1x RS485 Modbus digital I/O module (required for strict stall detection on all non-fan motors)
- 2x DIN contactors (or interface relays + contactors) for `PH` and `PWH` if not already installed
- 1x RS485 Modbus relay output module only if optional loads are kept (storage/crusher/heat gun)

## Purchase List (Portugal, tentative)

| Qty | Item | Target Price | Where to Buy |
|---:|---|---:|---|
| 1 | Waveshare `ESP32-S3-ETH-8DI-8RO` (RS485) | `~48-75 EUR` | Portugal distributors (Waveshare): Botnroll / PTRobotics |
| 1 | Modbus RTU Analog Output 8CH (`0-10V`) Waveshare 26211 | `24,90 EUR` | https://www.botnroll.com/pt/rs485/5373-conversor-industrial-din-modbus-rtu-para-sa-das-anal-gica-8-canais-output-0-10v-waveshare-26211.html |
| 1 | Modbus RTU Digital I/O 8CH configurable Waveshare 26244 | `26,50 EUR` | https://www.botnroll.com/pt/rs485/5382-conversor-industrial-din-de-8-ios-digitais-configur-vel-para-module-modbus-rtu-waveshare-26244.html |
| 1 | DIN PSU 24VDC 100W | `17,95 EUR` | https://www.botnroll.com/pt/acdc-24v/5481-fonte-de-alimenta-o-calha-din-24vdc-4-15a-100w.html |
| 1 | USB-TTL adapter FT232RNL | `9,50 EUR` | https://www.botnroll.com/pt/usb/6099-conversor-industrial-usb-para-ttl-com-chip-original-ft232rnl-e-circuitos-de-prote-o-waveshare-26738.html |
| 1 | DIN terminals + fuse holders + snubbers (allowance) | `25-40 EUR` | https://mauser.pt/material-electrico/quadros-e-distribuicao/ligacoes/bornes-din |

Procurement lock:
- Prefer `ESP32-S3-ETH-8DI-8RO` (RS485). Avoid `ESP32-S3-ETH-8DI-8RO-C` unless adding a CAN-to-RS485 gateway.
- For the analog module, confirm the voltage-output variant (`0-10V`), not the current-output (`0-20mA`) default variant.
- Use controller outputs only for contactor/relay coil control, not direct motor switching.
- Prefer `24VDC` coils on new contactors for pumps (`PH`, `PWH`).

Estimated control subtotal (without optional relay expansion): `~152-218 EUR`

## Web App Materials
- 1x Raspberry Pi 4 (or existing mini PC)
- 1x PSU + storage (if Pi)
- 1x small gigabit switch
- Cat6 patch cables

## Local HMI
- Preferred: reuse existing `20x4 I2C` LCD + 3 buttons (`UP`, `DOWN`, `OK/BACK`)
- Optional: 4.3" serial HMI touchscreen later

## Manual Override Hardware
- 3-position selector (`AUTO / OFF / REMOTE`)
- Manual reset pushbutton
- Emergency stop (replace only if needed)

## Wiring Materials
- `H07V-K 1.5 mm2` panel wire
- `H07V-K 2.5 mm2` for PE/heavier runs
- Shielded twisted pair for `RS485`
- Shielded instrument cable for analog/sensor runs
- Ferrules and DIN terminal blocks

## Strict Stall Detection (practical)
- Prefer existing overload relay auxiliary contacts (`95-96` / `97-98`) into DI
- If unavailable, add one current monitoring relay per critical motor
- For single-phase motors, pass only phase (`L`) through CT window
- Use detector relay output (`NO/NC`) into DI module

## Remaining Open Items
1. Existing contactor coil voltage audit (old panel components still to verify)
2. Thermostat type (dry contact or powered output)
3. Sensor electrical type (`NTC10K`, `PT100`, `PT1000`, other)
4. Optional points in use: `PT100/PT1000`, `NRC/IR`
5. Fan motor current and capacitor from nameplate

## Serial Overrides
- Force flame detected: `FLAME ON`
- Return flame logic to sensor: `FLAME AUTO`
- Force run stage: `STAGE 0`, `STAGE 1`, `STAGE 2`, `STAGE 3`
- Return run stage to temperature-based logic: `STAGE AUTO`
- Manual fan override in REMOTE mode: `FAN 1` ... `FAN 100`
- Return fan to boiler automatic profile: `FAN AUTO`
- Stage override is applied in `RUN` mode and used automatically when controller reaches `RUN`

## References
- Waveshare controller (RS485): https://www.waveshare.com/esp32-s3-eth-8di-8ro.htm
- Waveshare controller (CAN variant): https://www.waveshare.com/esp32-s3-eth-8di-8ro-c.htm
- Waveshare distributors: https://www.waveshare.com/distributors
- NPBC-V3M technical manual: https://www.naturela-bg.com/files/NPBC-V3M-1_TM_2_1_EN.pdf
- NPBC-V3M user guide: https://www.naturela-bg.com/files/NPBC-V3M-1_rev2_1_EN.pdf
