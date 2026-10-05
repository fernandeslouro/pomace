# NPBC V3M Replacement BOM

## Scope
Control hardware and panel integration materials for the NPBC-V3M replacement controller.
This file tracks the practical shopping list and wiring assumptions used by the firmware profile in `algorithm.ino`.

First replacement scope (user clarification, 2026-10-03): retain the currently connected FM, SF, PH and PWH functions. The big-storage-to-small-hopper transfer motor is a future extension; defer its sensors, control logic and dedicated expansion purchases. Do not assume Naturela SB (internal auger) represents that extension.

Implementation caveat: the existing firmware pin map is not a completed Waveshare board integration. Fan output currently uses `analogWrite` via `FanPwmDriver`, not a verified mains fan driver or Modbus analog output. Board I/O mapping, sensor interfaces and fan power control must be implemented and checked before replacing the Naturela board.

## Baseline Control Hardware

| Qty | Item | Target Price (EUR) | Role | Source |
|---:|---|---:|---|---|
| 1 | Waveshare `ESP32-S3-ETH-8DI-8RO` (RS485 version) | 48-75 | Main controller (Ethernet + DI/RO + RS485 Modbus) | Waveshare distributors (Botnroll / PTRobotics) |
| 1 | Waveshare 26211 Modbus RTU Analog Output 8CH (`0-10V`, voltage-output variant) | 24.90 | Fan speed command output | https://www.botnroll.com/pt/rs485/5373-conversor-industrial-din-modbus-rtu-para-sa-das-anal-gica-8-canais-output-0-10v-waveshare-26211.html |
| 1 | Waveshare 26244 Modbus RTU 8CH configurable digital I/O | 26.50 | Stall/fault digital inputs expansion | https://www.botnroll.com/pt/rs485/5382-conversor-industrial-din-de-8-ios-digitais-configur-vel-para-module-modbus-rtu-waveshare-26244.html |
| 1 | DIN PSU 24VDC 100W | 17.95 | Control supply | https://www.botnroll.com/pt/acdc-24v/5481-fonte-de-alimenta-o-calha-din-24vdc-4-15a-100w.html |
| 2 | DIN contactor, coil `24VDC` (for `PH` and `PWH`) | 12-30 each | Interposing motor switching stage (controller output -> coil only) | Local distributor (Schneider/CHINT/ABB/Siemens equivalent) |
| 1 | USB-TTL adapter FT232RNL | 9.50 | Commissioning and recovery access | https://www.botnroll.com/pt/usb/6099-conversor-industrial-usb-para-ttl-com-chip-original-ft232rnl-e-circuitos-de-prote-o-waveshare-26738.html |
| 1 | DIN terminals, fuse holders, RC snubbers (allowance) | 25-40 | Panel wiring and protection | https://mauser.pt/material-electrico/quadros-e-distribuicao/ligacoes/bornes-din |

Estimated control subtotal (without optional relay expansion): 152-218 EUR. This provisional estimate excludes the replacement fan power controller; the listed analog output module is on hold.

## Fan Selection Update (2026-10-03)

### Lower-cost alternative researched after budget objection

United Automation `FSC230/10`, SKU `A72290`, is listed directly by its manufacturer at GBP 80 excluding VAT. It is an industrial 230VAC phase-angle controller for induction motors/fans, with isolated analog input, built-in RC snubber and RFI filter. Use Waveshare `26211` voltage-output module for the 0–10V command; no FSP-L15 cable is required. Manufacturer advertises free UK/EU delivery above GBP 50, but exports are DAP: Portugal import VAT and possible clearance charges are additional, and the final delivered total has not been verified.

This is a lower-cost candidate, not an equally documented substitute for the Alco arrangement-specific support: the FSC datasheet does not explicitly cover a three-phase motor converted to single-phase operation with a Steinmetz capacitor. Minimum operating load is 200mA; actual running current at intended speeds, startup and temperature need checking. Its voltage input is 5kΩ (2mA at 10V); confirm analog-module drive capability before finalizing interface wiring. Keep the Alco option as the stronger documented fallback; do not mark either as tested on this exact motor.

Sources checked 2026-10-03:
- https://united-automation.com/product/fsc230-10-fan-speed-controller-10a/
- https://united-automation.com/wp-content/uploads/2025/07/X10784-FAN-SPEED-CONTROLLES-FSC230-10-United-Automation-11.pdf
- https://united-automation.com/delivery-returns/

### Earlier arrangement-specific selection

Supersedes the Sentera shortlist and analog-module hold below: select Alco/Emerson `FSP-150` power module, manufacturer part `800370`, plus `FSP-L15` signal cable `804693` and Waveshare `26211` / Analog Output 8CH (B), voltage-output 0–10V version. The Alco manufacturer's documentation explicitly addresses Steinmetz-capacitor motor loads, with FSP-150 nominal current range 0.3–5A and maximum 80% of rated current for that arrangement. This is stronger documented motor-arrangement support than the MVS candidate. Existing two-wire supply, delta/star plate and capacitor are consistent with Steinmetz operation, but internal winding connections remain an inference; commissioning must check actual supply current, reliable starting, temperature and usable low-speed airflow. Retain the existing motor/capacitor wiring.

FSP-150 is a legacy product: a Copeland bulletin describes replacement by PKE-6 in their refrigeration units, so do not assume ongoing production or that that replacement bulletin certifies this boiler motor. A new-condition surplus listing at Industrietec shows EUR 210.08 excluding VAT and one unit; Portugal shipping is not verified. FSP-L15 is listed at Frigopartners for EUR 15.41 including displayed VAT, destination taxes/shipping to be checked. Botnroll Portugal lists 26211 for EUR 24.90 including VAT. This fan kit substantially exceeds the earlier whole-control budget; there is no verified final delivered price.

Sources:
- Alco manufacturer instructions (hosted by distributor): https://www.alfaco.cz/alco/navody/fsp.pdf
- Alco/Emerson catalogue (manufacturer-authored, distributor-hosted): https://cdn1.npcdn.net/attachments/1402995228a78b7a609a7484cab2bda1e6d3aad9cf.pdf
- Copeland replacement bulletin: https://media.copeland.com/921188dd-a11e-4aa5-84e4-b29800c3f234/TI_Unit_Fan_03_EN_FSP_Replacement.pdf
- https://www.industrietec-automation.com/EMERSON-FSP-150-Leistungsteil-fuer-Drehzahlregler-PCN-800370
- https://frigopartners.com/alco-fsp-l15-804693-cable-with-plug-for-fsp-1.5-m
- https://www.botnroll.com/pt/rs485/5373-conversor-industrial-din-modbus-rtu-para-sa-das-anal-gica-8-canais-output-0-10v-waveshare-26211.html

### Historical shortlist (superseded)

Research update: preferred command architecture is direct RS485 Modbus, not a separate 0–10V output. Candidate: Sentera `MVS-1-15CDM`, a 230VAC, 1.5A DIN-rail phase-angle fan controller. Its register map confirms direct output control (holding registers 7/8 enable Modbus/output override; 31 requests OFF or 30–100% supply voltage). Thus Waveshare 26211 is unnecessary for this candidate. It does not reproduce Naturela percentages directly; minimum airflow, reliable restart, motor current/temperature and communications-loss shutdown need commissioning checks. No documented communications watchdog was found in the register map; do not assume loss of RS485 stops the fan.

Compatibility remains unverified: the photographed delta/star nameplate is a three-phase motor rating, while the existing two-wire supply and capacitor suggest operation from single-phase power through a capacitor arrangement. Sentera specifies single-phase voltage-controllable motors, not explicitly this converted configuration. Seek supplier/manufacturer confirmation for this exact motor arrangement before purchase; current rating alone is insufficient. Preserve the existing capacitor wiring. Price and Portugal delivery have not been verified.

Sources checked 2026-10-03:
- https://www.sentera.eu/en/productdetails/ac-fan-speed-controller-0-10-v-din-rail-15-a/119383
- https://www.sentera.eu/en/files/article/document/mbrm/mvs-1-15cdm-modbus-register-map.pdf
- https://www.sentera.eu/en/files/article/document/miweb-en/mvs-1-15cdm-mounting-instruction.pdf

User reports `FM`, `SF`, `PWH`, `PH` connected and `FSG`, `SB`, `IGN`, `FC` unconnected on the Naturela board. `ACF` status and cable destinations are unconfirmed. The fan is `90W`, with nameplate `220V Δ / 380V Y`, `0.53A / 0.32A`, and a connected `4 µF ±5%` capacitor. Its speed changes with the Naturela 0–100 setting.

Do not purchase Waveshare 26211 for the fan yet. Select a compatible motor power controller and confirm its command interface first. FM is a mains output, distinct from optional ACF analog control; an analog output module alone cannot replace the FM power stage. See `CURRENT_HARDWARE_INVENTORY.md` for the inspection notes. Firmware SB mapping remains present despite the reported unused terminal and needs review before commissioning.

Procurement lock:
- Buy `ESP32-S3-ETH-8DI-8RO` (RS485). Do not buy `ESP32-S3-ETH-8DI-8RO-C` unless a CAN-to-RS485 gateway is intentionally added.
- Only if the selected fan power controller requires `0-10V`, confirm the analog module is the voltage-output variant (not `0-20mA`); purchase is currently on hold.
- Do not connect motor loads directly to controller outputs. Use contactor/interposing stage for every motor branch.

## Optional Expansion

| Qty | Item | Notes |
|---:|---|---|
| 1 | RS485 Modbus relay output module | Only needed if optional loads remain (storage, crusher, heat gun). |
| 1 | 20x4 I2C LCD + 3 buttons | Reuse existing local HMI where possible. |
| 1 | Raspberry Pi 4 or mini PC | Web app host (outside panel control budget). |
| 1 | Small gigabit switch + Cat6 patch cables | Local network integration. |

## Wiring Notes

1. Keep power domains separated: `230 VAC` power wiring apart from `24 VDC` control and low-level signals.
2. Use shielded twisted pair for `RS485` and terminate bus ends correctly.
3. Use shielded instrument cable for analog/sensor runs (`0-10V`, temperature, flame input).
4. Firmware assumes thermostat demand input is active-low dry contact (contact to GND means demand).
5. Firmware safety chain is enabled by default (`high-limit`, `backfire`, `E-stop`) and latches lockout until reset.
6. Stall detection is enabled for all non-fan motors using overload/current-monitor contact signals.
7. Controller outputs must drive only contactor/relay coils. Motor power must be switched by contactors.
8. Prefer `24VDC` coils on new contactors (`PH`, `PWH`) and add coil suppression (diode for DC coils / RC for AC coils).
9. If legacy contactor coils are `230 VAC`, use interposing relays/interface stage before controller outputs.
10. Use ferrules on all stranded conductors and fuse each control branch according to local code.

## Firmware Alignment (Current Profile)

- Tunables file: `control_config.h`
- Hardware map file: `hardware_profile.h`
- Profile name: `NPBC_PANEL_V1`
- Mode selector pins: enabled
- Safety chain pins: enabled
- Stall inputs: enabled
- Output mapping currently active in firmware:
  - `SF`: pin `8`
  - `PH`: pin `10`
  - `PWH`: pin `9`
  - `SB`: pin `11`
- Safety inputs:
  - `AUTO` selector: pin `22`
  - `REMOTE` selector: pin `23`
  - `HIGH_LIMIT`: pin `24`
  - `BACKFIRE`: pin `25`
  - `E_STOP`: pin `26`

## References

- Controller (RS485): https://www.waveshare.com/esp32-s3-eth-8di-8ro.htm
- Controller (CAN variant): https://www.waveshare.com/esp32-s3-eth-8di-8ro-c.htm
- Distributors: https://www.waveshare.com/distributors
- NPBC-V3M technical manual: https://www.naturela-bg.com/files/NPBC-V3M-1_TM_2_1_EN.pdf
- NPBC-V3M user guide: https://www.naturela-bg.com/files/NPBC-V3M-1_rev2_1_EN.pdf
