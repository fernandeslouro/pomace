# Current Hardware Inventory (from photos)

Date of analysis: 2026-02-13

## Confirmed Components

| Component | Model / Marking | Electrical Data Readable | Confidence | Evidence |
|---|---|---|---|---|
| Boiler controller PCB | Naturela Ltd `NPBC-V3M-1.1` | Board markings: `COM1`, `COM2`, `3.3V`, `10V`, `LED`, `5A` (fuse marking on board) | High | `img/IMG_5072.jpg`, `img/IMG_5066.jpg` |
| Main RCCB / differential switch | GE `NID4` | `In=63A`, `IΔn=0.03A`, `Un=230/400V~`, `4POLE`, TEST button | High | `img/IMG_5069.jpg`, `img/IMG_5066.jpg` |
| Contactor | Schneider Electric `LC1D09` | Aux contacts marked `13-14 (NO)`, `21-22 (NC)`, coil `A1/A2` terminals visible | High | `img/IMG_5069.jpg` |
| MCB (small, single pole) | `CHINT/CHNT C10` | `C10` visible (nominal 10A curve C) | High | `img/IMG_5069.jpg` |
| Fan motor | Ref `00QW3530` (label on motor) | `220V Δ / 380V Y`, `0.53A / 0.32A`, `0.09 kW`, `2800 rpm`, `IP44`, `InsCl B`, working temp `-40/+40 C`, `Made in Spain` | High | `img/fan-motor.jpg` |

## Additional Motor Found (separate photo)

| Component | Model / Marking | Electrical Data Readable | Confidence | Evidence |
|---|---|---|---|---|
| Motor (likely another load, not the fan) | Swedrive AB, type appears `DPHI 71B-4` | Plate partially readable: `0.37 kW` at `50 Hz`, approx `220-240V Δ / 380-420V Y`, approx `2.20A / 1.25A`; also `60 Hz` row visible with `440V Y` and approx `0.42 kW` | Medium | `img/IMG_5064.jpg` |

## Panel-Level Observations

| Observation | Confidence | Evidence |
|---|---|---|
| Wiring appears to include phase conductors + neutral + protective earth (PE) | Medium | `img/IMG_5066.jpg`, `img/IMG_5069.jpg` |
| There is at least one additional multi-pole breaker in the middle (rating/model not readable) | High | `img/IMG_5069.jpg`, `img/IMG_5066.jpg` |
| Existing architecture is legacy NPBC-V3M controller with direct terminal wiring | High | `img/IMG_5066.jpg`, `img/IMG_5072.jpg` |

## External Distribution Board (new photo `IMG_5073`)

| Component | Model / Marking | Electrical Data Readable | Confidence | Evidence |
|---|---|---|---|---|
| External enclosure | Brand appears `Jengar/Jangar`, marking appears `485902`, `IP65` | Wall-mounted protective enclosure | Medium | `img/IMG_5073.jpg` |
| Upstream RCCB in external board | GE `NID4` | Same as internal photo: `In=63A`, `IΔn=0.03A`, `Un=230/400V~`, `4POLE` | High | `img/IMG_5073.jpg` |
| Hager 3-pole MCB | `MW340`, code appears `453340`, curve `C40` | 3-pole branch protection (likely feeder) | High | `img/IMG_5073.jpg` |
| Hager single-pole MCB (middle) | Marking appears `MW116`, `C16` (blurred) | Likely 1P, 16A curve C | Medium | `img/IMG_5073.jpg` |
| Hager single-pole MCB (right) | Marking appears `MW110`, code appears `453110`, `C10`, `230/400V~` | 1P branch protection | High | `img/IMG_5073.jpg` |
| External 2-gang switch/push station below board | `IP54`; icons suggest lighting/aux function | Two-control wall device; exact function not readable | Medium | `img/IMG_5073.jpg` |

## Unknown / Still Needed

1. External middle Hager MCB exact reference (confirm if `MW116 C16`).
2. `LC1D09` coil voltage (critical for replacement output interface design).
3. Full, clean type line for the Swedrive motor plate (to avoid wrong assumptions).
4. Full fan motor top line (if manufacturer/type field exists above the visible rows).
5. Exact function of the external `IP54` 2-gang switch station.

## Minimum Extra Photos To Close Gaps

1. Tight, front-on photo of `LC1D09` side label (coil voltage line).
2. Tight, front-on photo of external middle Hager breaker label.
3. Tight, front-on photo of the external `IP54` switch labels/icons.
4. One sharp photo per motor plate with no glare and perpendicular angle.
