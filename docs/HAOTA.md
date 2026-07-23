# Home Assistant OTA (draft)

Production-like Matter OTA via Home Assistant and DCL TestNet. **Work in progress** — merge into [TESTING.md](TESTING.md) after validation.

Validate [local chip-tool OTA](TESTING.md#local-matter-ota-chip-tool--ota-provider) first.

## Prerequisites


| **Requirement**                                                                           | **Why**                                                                        |
| ----------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------ |
| Device commissioned **to HA’s fabric**                                                    | OTA runs on that fabric                                                        |
| **IPv6 enabled** on HA                                                                    | Matter needs it                                                                |
| For Thread devices: **HA as border router** (SkyConnect/ZBT-2 + OTBR app), not Apple-only | Apple OTBR often breaks HA OTA (`Target node did not process the update file`) |
| Matter Server **beta (matter.js)** if updates fail                                        | Fixes many HA OTA provider issues                                              |


## Steps

1. **Commission device to HA** (Companion app → Settings → Matter → Add device).
2. Confirm an **Update** entity appears under the device (proves OTA Requestor is visible).
3. Read the device’s **Vendor ID, Product ID, software version** (HA device info / diagnostics).
4. **Register on DCL TestNet** ([testnet.iotledger.io](https://testnet.iotledger.io/models)):
   - Matching VID/PID
   - New software version (e.g. `2`)
   - URL to your hosted `*-ota.bin` (HTTPS, reachable from HA)
5. Flash **v1** on the device, commission to HA.
6. Build **v2** OTA image (`build/*-ota.bin`; same version workflow as [TESTING.md](TESTING.md#step-1--version-strategy-important)).
7. Force update check in HA:

```yaml
service: homeassistant.update_entity
target:
  entity_id: update.<your_device_update_entity>
```

Or use **Settings → Updates**.

8. Install when HA offers the update; watch serial logs for the same `cosmos_matter_events` OTA messages as in [TESTING.md](TESTING.md#step-6--watch-transfer-and-reboot).

---

## **Network notes for your three apps**


| **App**                  | **Radio**           | **HA OTA difficulty**                                            |
| ------------------------ | ------------------- | ---------------------------------------------------------------- |
| `iotBasicBinarySensor`   | Wi-Fi               | Easiest — start here                                             |
| `iotEnvironmentalSensor` | Wi-Fi (C5)          | Same pattern; use `idf.py --preview` for C5                      |
| `iotDualModeBtn`         | Thread (sleepy MTD) | Harder — needs working Thread + OTBR; sleepy device may slow OTA |


For Thread OTA via HA:

- Use **HA OTBR**, not only HomePod/Nest as border router.
- Keep the device awake during OTA (powered, not deep sleep).
- If OTA fails with provider errors, switch Matter Server to **beta (matter.js)** in the add-on config.

---

## **What “success” looks like**

1. HA shows an **Update** entity after commissioning.
2. Serial log: `OTA download in progress` → `OTA download complete` → `OTA apply in progress` → `OTA apply complete`.
3. Device reboots and reports the **new software version** in HA device info.

---

## Practical recommendation

1. **First:** Local chip-tool OTA with `iotBasicBinarySensor` ([TESTING.md](TESTING.md)).
2. **Then:** This HA + DCL path once TestNet entry and image hosting are in place.
3. Use HA mainly for **commissioning + normal control** until DCL is configured.