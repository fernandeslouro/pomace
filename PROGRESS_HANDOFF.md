# Progress handoff — 2026-10-04

## Objective and user decisions

Replace the Naturela NPBC-V3M boiler controller in Portugal, retaining the existing fan motor and capacitor. The immediate goal is an affordable, complete hardware purchase list. The user wants us to choose the fan-control hardware and research compatibility, rather than ask them to choose a control protocol. Nothing has been ordered.

Keep FM, SF, PH and PWH functionality. The big-storage-to-small-hopper transfer motor is a future extension; its sensors and logic are deferred. Naturela SB means the internal auger and should not automatically be equated with that extension.

## Inspection findings preserved

- User reports FM, SF, PWH and PH connected; FSG, SB, IGN and FC unconnected. These are output terminals. Cable destinations and ACF status remain unconfirmed.
- Fan has two supply wires connected to FM. Its speed changes with the Naturela panel's 0–100 setting.
- Photographed motor reference: 00QW3530; 90W, 220V delta / 380V star, 0.53A / 0.32A, 2800rpm, 50Hz, IP44, insulation class B.
- Existing Ducati Energia capacitor: 4µF ±5%, marked 425/475/500VAC with different endurance ratings. Retain it and the motor.
- Delta/star plate plus two-wire operation and capacitor suggest a Steinmetz arrangement; internal winding connections have not been inspected. This remains an inference.

## Fan procurement research and latest outcome

The last user request was for a cheaper alternative to the Alco kit. We found United Automation FSC230/10, SKU A72290, at GBP 80 excluding VAT direct from the manufacturer. It is a 230VAC industrial phase-angle controller for induction motor/fan loads, with isolated analog input, RC snubber and RFI filtering. Proposed command module is Waveshare 26211 / Analog Output 8CH (B), 0–10V voltage-output version, listed at Botnroll Portugal for EUR 24.90 including VAT. No Alco signal cable is needed for FSC.

FSC documentation does not explicitly confirm a three-phase motor operated through a Steinmetz capacitor. Therefore it is a credible budget candidate, not a fully verified purchase. Minimum operating current is 200mA; verify actual motor current at intended speeds. Voltage input resistance is 5kΩ (2mA at 10V); analog-module drive capability still needs confirmation. Starting, temperature and minimum reliable airflow need commissioning checks.

Manufacturer advertises free UK/EU delivery above GBP 50, but exports are DAP with import taxes and possible clearance charges borne by recipient. Final delivered Portugal price and stock/lead time are not verified.

Earlier Alco/Emerson FSP-150 (800370) has explicit manufacturer documentation for Steinmetz-capacitor loads, 0.3–5A nominal range and 80% current derating for that arrangement. It requires FSP-L15 cable (804693) and the same analog module. Listed controller price was EUR 210.08 excluding VAT; kit around EUR 300 before shipping. It is a legacy product and too costly for the user's preference. Keep as documentary fallback, not an approved order.

Sentera MVS-1-15CDM was investigated earlier: direct RS485 Modbus output control is documented, but support for this converted motor arrangement is not explicit. No communications watchdog was established. This shortlist was superseded; do not silently reinstate it as proven compatible.

Sources and dated price findings are in [NPBC_V3M_REPLACEMENT_BOM.md](NPBC_V3M_REPLACEMENT_BOM.md). The capacitor/motor arrangement is the unresolved compatibility question; a high current rating alone does not resolve it.

## Remaining work when resuming

1. Resolve exact fan-controller compatibility and command-module electrical loading, and verify an affordable Portugal delivered purchase route. Do not claim manufacturer confirmation has been received; no supplier messages have been sent.
2. Finish the complete BOM: inventory sensor connections/electrical characteristics, reusable contactor count and coil voltages, supply sizing and panel protection. Main Waveshare board selection is ESP32-S3-ETH-8DI-8RO with RS485, not the CAN variant.
3. Implement the actual board integration. Current firmware is a scaffold with generic pins, analogWrite fan PWM and DallasTemperature/OneWire sensing; it does not yet implement Waveshare I/O, Modbus analog fan command or Naturela NTC10K sensing.
4. Review independent safety cutoff and behavior on controller/communications failure before replacing the operating Naturela board.

See [PURCHASE_READINESS.md](PURCHASE_READINESS.md) for current purchase limitations and [CURRENT_HARDWARE_INVENTORY.md](CURRENT_HARDWARE_INVENTORY.md) for inspection evidence. The old EUR 152–218 subtotal excludes the fan power controller and is not a complete order-ready budget.

## Repository checkpoint

Branch: feature/npbc-v3m-control-v1. This checkpoint contains documentation only; no firmware behavior was changed. Validation: git diff --check. User requested committing and pushing progress before shutdown.
