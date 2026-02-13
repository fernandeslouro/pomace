# NPBC V3M Replacement BOM

## Scope
Control hardware and panel integration materials for the NPBC-V3M replacement controller.
This file tracks the practical shopping list and wiring assumptions used by the firmware profile in `algorithm.ino`.

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

Estimated control subtotal (without optional relay expansion): 152-218 EUR.

Procurement lock:
- Buy `ESP32-S3-ETH-8DI-8RO` (RS485). Do not buy `ESP32-S3-ETH-8DI-8RO-C` unless a CAN-to-RS485 gateway is intentionally added.
- Confirm analog output module is the `0-10V` voltage-output variant for fan command (not `0-20mA` current-output variant).
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
