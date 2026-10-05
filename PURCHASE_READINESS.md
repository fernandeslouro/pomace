# Board replacement purchase readiness — 2026-10-03

Scope: replace Naturela for FM, SF, PH and PWH; retain fan motor and capacitor. Storage extension deferred.

## Confirmed core selection

| Quantity | Item | Purpose |
|---|---|---|
| 1 | Waveshare ESP32-S3-ETH-8DI-8RO, standard Ethernet version with isolated RS485 | Programmable main controller; 8 relay outputs and 8 digital inputs |
| 1 | USB-A/USB-C or USB-C/USB-C data cable suitable for commissioning computer | Firmware programming and serial connection; no separate USB-TTL adapter needed for normal USB commissioning |

Main board source: https://www.waveshare.com/catalog/product/view/id/7372/s/esp32-s3-eth-8di-8ro/category/564/
Documentation: https://www.waveshare.com/wiki/ESP32-S3-ETH-8DI-8RO

## Items that prevent a complete, order-ready BOM

- Fan power controller: Alco/Emerson FSP-150 `800370`, cable FSP-L15 `804693`, and Waveshare 26211 voltage-output module have the stronger documented Steinmetz-capacitor support. Budget alternative researched: United Automation FSC230/10 `A72290`, GBP 80 excluding VAT direct, using the same analog module without the Alco cable. FSC supports induction motor loads but does not explicitly document this converted motor arrangement; exact compatibility and final Portugal delivered cost remain unverified. See the evidence and limitations in the BOM.
- Sensor acquisition: main board has digital inputs, not ready-to-use NTC/photoresistor inputs. Naturela manual specifies NTC10K temperature sensing and a photo sensor; connected sensor terminals and installed sensor electrical characteristics still need inventory. Current firmware uses DallasTemperature/OneWire, so it does not read existing NTCs as written. Do not order generic analog modules as though they directly accept these sensors.
- Motor switching: coil voltage and number of reusable contactors are unverified. Need appropriately rated interfaces/contactors for SF, PH and PWH; do not specify quantities or coil suppression types without those facts.
- Power supply and panel protection: size after interface coils and sensor acquisition are specified. Controller takes 7–36V DC. Mean Well HDR-30-24 (24V/1.5A) is a candidate, not a verified full-panel sizing decision.
- Independent safety and watchdog: confirm existing high-limit/backfire/E-stop protections and specify power cutoff on controller failure; RS485 loss must not be assumed to stop the fan.

## Remove from automatic ordering

Storage expansion purchases, automatic purchase of two pump contactors, and the legacy estimated complete budget. Waveshare 26211 is now included for the selected FSP-150 fan controller. Existing BOM is not a complete replacement kit.

No full replacement purchase list is approved by this document. Firmware integration and commissioning remain necessary after hardware selection.
