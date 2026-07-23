# Hardware and OTA testing

Manual test procedures for Cosmos Matter firmware. Validated on **`iotBasicBinarySensor`** (XIAO ESP32-C6, Wi‑Fi Matter); the same OTA flow applies to the other apps once their OTA images are built.

See also: [BUILD.md](BUILD.md) (toolchain, flash), [HARDWARE.md](HARDWARE.md) (GPIO, factory reset button).

---

## Local Matter OTA (chip-tool + OTA provider)

Use this for day-to-day firmware iteration. **Home Assistant cannot push arbitrary local `.bin` files** — it relies on the CSA Distributed Compliance Ledger (DCL). See [Home Assistant OTA (planned)](#home-assistant-ota-planned) below.

### Roles

| Component | Tool | Typical node ID | Purpose |
|-----------|------|-----------------|--------|
| Commissioner / trigger | `chip-tool` | `0x1B669` (112233) | Commission devices, run `announce-otaprovider` |
| OTA provider | `chip-ota-provider-app` | `0xDEADBEEF` | Answers `QueryImage`, streams firmware over BDX |
| OTA requestor | ESP firmware | `0x18` (your choice at commission time) | Downloads and applies the OTA image |

All three must be on the **same Matter fabric**. For the simplest first test, factory-reset the ESP and commission it **only** with chip-tool (add Home Assistant later as a second admin if needed).

### Prerequisites

- ESP-IDF + esp-matter installed ([BUILD.md](BUILD.md))
- Device and PC on the same Wi‑Fi/LAN (IPv6/mDNS reachable)
- BLE available on the PC for `pairing ble-wifi` (ESP32-C6 Wi‑Fi commissioning)
- OTA requestor enabled in firmware (`CONFIG_ENABLE_OTA_REQUESTOR=y` in shared defaults; `cosmos_matter_ota_configure()` after `esp_matter::start()`)

**Cosmos test device IDs** (from commissioning logs): Vendor ID **65521** (`0xFFF1`), Product ID **32768** (`0x8000`).

### Step 0 — Build host tools (once)

`chip-tool` and `chip-ota-provider-app` are separate builds under esp-matter:

```bash
cd $ESP_MATTER_PATH/connectedhomeip/connectedhomeip

# chip-tool (if not already in out/host/)
scripts/examples/gn_build_example.sh examples/chip-tool out/host chip_config_network_layer_ble=false

# OTA provider (not built with chip-tool)
scripts/examples/gn_build_example.sh examples/ota-provider-app/linux out/host chip_config_network_layer_ble=false
```

Binaries:

```text
$ESP_MATTER_PATH/connectedhomeip/connectedhomeip/out/host/chip-tool
$ESP_MATTER_PATH/connectedhomeip/connectedhomeip/out/host/chip-ota-provider-app
```

Handy exports:

```bash
export CHIP_TOOL="$ESP_MATTER_PATH/connectedhomeip/connectedhomeip/out/host/chip-tool"
export CHIP_OTA_PROVIDER="$ESP_MATTER_PATH/connectedhomeip/connectedhomeip/out/host/chip-ota-provider-app"
export CHIP_STORAGE=/tmp/chip_tool_kvs          # chip-tool fabric state
export PROVIDER_KVS=/tmp/chip_kvs_provider      # OTA provider fabric state (separate!)
```

Use **`--storage-directory $CHIP_STORAGE`** on every chip-tool command so fabric state stays consistent. Do **not** point chip-tool and the provider at the same KVS file.

To start clean:

```bash
rm -f $CHIP_STORAGE $CHIP_STORAGE/chip_* /tmp/chip_config.ini /tmp/chip_factory.ini /tmp/chip_counters.ini
rm -f $PROVIDER_KVS $PROVIDER_KVS/chip_*
```

### Step 1 — Version strategy (important)

Matter OTA only applies when the offered image is **newer** than what is running.

Example workflow for `iotBasicBinarySensor`:

| Artifact | `PROJECT_VER` (CMakeLists) | `PROJECT_VER_NUMBER` | `CONFIG_DEVICE_SOFTWARE_VERSION_NUMBER` |
|----------|----------------------------|----------------------|----------------------------------------|
| Image **on device** (flash) | `1.0` | `1` | `1` |
| **OTA file** (build output) | `1.1` | `2` | `2` |

1. Set the **lower** version in `iotBasicBinarySensor/CMakeLists.txt` and `sdkconfig.defaults`, then `idf.py build flash`.
2. Bump to the **higher** version in both files, rebuild **without flashing** — only the OTA artifact is needed for the update.

Ensure OTA image generation is on in `sdkconfig.defaults`:

```text
CONFIG_CHIP_OTA_IMAGE_BUILD=y
CONFIG_DEVICE_SOFTWARE_VERSION_NUMBER=2   # must match PROJECT_VER_NUMBER for OTA build
```

If you change `sdkconfig.defaults` but already have a stale `sdkconfig`, regenerate:

```bash
cd iotBasicBinarySensor
rm -f sdkconfig sdkconfig.old
idf.py set-target esp32c6
idf.py build
```

OTA output:

```text
iotBasicBinarySensor/build/iotBasicBinarySensor-ota.bin
```

### Step 2 — Commission the ESP (requestor)

Factory-reset if needed (long-press GPIO9, or `idf.py erase-flash`). Confirm the serial log shows a commissioning window and BLE advertising.

**Syntax** (note argument order — setup PIN is *not* before SSID):

```text
chip-tool pairing ble-wifi <node-id> <ssid> <wifi-password> <setup-pin-code> <discriminator>
```

Example (quote SSID/password when they contain spaces):

```bash
$CHIP_TOOL --storage-directory $CHIP_STORAGE \
  pairing ble-wifi 0x18 "Your WiFi SSID" "your-wifi-password" 20202021 3840
```

Defaults for esp-matter examples: setup PIN **20202021**, discriminator **3840** (serial log shows `discriminator=3840/15`).

Verify connectivity before OTA:

```bash
$CHIP_TOOL --storage-directory $CHIP_STORAGE \
  basicinformation read software-version 0x18 0
```

This must return quickly (no mDNS timeout). If it times out, see [Troubleshooting](#troubleshooting).

### Step 3 — Start the OTA provider

In a dedicated terminal (leave it running):

```bash
$CHIP_OTA_PROVIDER --KVS $PROVIDER_KVS \
  -f /path/to/cosmosFivePieceBasis/iotBasicBinarySensor/build/iotBasicBinarySensor-ota.bin
```

On first use, commission the provider onto the **same fabric** as chip-tool. Use the discriminator/passcode printed when the provider starts (often `20202021` / `3840`):

```bash
$CHIP_TOOL --storage-directory $CHIP_STORAGE \
  pairing onnetwork-long 0xDEADBEEF 20202021 3840
```

### Step 4 — ACL on the provider

The provider needs an ACL entry so requestors can call the OTA Provider cluster (`0x0029`). If the provider was just commissioned, extend the existing ACL (do not replace admin entries). Example from the [upstream OTA provider README](https://github.com/project-chip/connectedhomeip/blob/master/examples/ota-provider-app/linux/README.md):

```bash
$CHIP_TOOL --storage-directory $CHIP_STORAGE accesscontrol write acl \
  '[{"fabricIndex": 1, "privilege": 5, "authMode": 2, "subjects": [112233], "targets": null}, {"fabricIndex": 1, "privilege": 3, "authMode": 2, "subjects": null, "targets": [{"cluster": 41, "endpoint": null, "deviceType": null}]}]' \
  0xDEADBEEF 0
```

- `112233` decimal = chip-tool node `0x1B669`
- Cluster `41` = `0x0029` (OTA Provider)
- Adjust `fabricIndex` if your fabric index is not `1` (`accesscontrol read acl 0xDEADBEEF 0`)

### Step 5 — Announce provider to the requestor

```bash
$CHIP_TOOL --storage-directory $CHIP_STORAGE \
  otasoftwareupdaterequestor announce-otaprovider 0xDEADBEEF 0 0 0 0x18 0
```

Arguments: provider node ID, vendor ID (`0` = wildcard), announcement reason, metadata, **requestor node ID**, requestor endpoint.

Success looks like `status = 0x00 (SUCCESS)` in chip-tool and BDX block exchange in the provider log.

### Step 6 — Watch transfer and reboot

**Provider terminal:** repeated `BDX:Block` / `BDX:BlockQuery` messages to node `0x18`.

**ESP serial:** CHIP BDX traffic, then (via `cosmos_matter_events`):

```text
I cosmos_matter_events: OTA download in progress
I cosmos_matter_events: OTA download complete
I cosmos_matter_events: OTA apply in progress
I cosmos_matter_events: OTA apply complete — reboot pending
```

A ~1.6 MB image typically takes **8–12 minutes**. The device reboots when apply completes.

Verify the new version:

```bash
$CHIP_TOOL --storage-directory $CHIP_STORAGE \
  basicinformation read software-version 0x18 0
```

Expect `SoftwareVersion` **2** and string **1.1** when using the example version table above.

### Troubleshooting

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| `pairing ble-wifi` usage / `Invalid argument setup-pin-code` | Wrong argument order or extra `20202021` | Use `node-id ssid password 20202021 3840` only |
| `announce-otaprovider` or attribute read **Timeout** | chip-tool fabric ≠ device fabric (stale KVS) | Clear `$CHIP_STORAGE`, factory-reset ESP, re-commission with chip-tool only |
| mDNS `No memory` noise on chip-tool | Busy LAN (HA, Hue, etc.) | Usually non-fatal; fix fabric first |
| `CASE failed to match destination ID with local fabrics` during OTA | Another controller probing wrong fabric | Harmless if BDX continues |
| OTA never starts after announce | Provider ACL missing | Re-run ACL write (Step 4) |
| Download completes but no apply | Image version ≤ running version | Flash older base firmware; rebuild OTA with higher `PROJECT_VER_NUMBER` |
| Provider log silent after announce | Provider not on same fabric or wrong `-f` path | Re-commission provider; check OTA file path |

**Multi-fabric (HA + chip-tool):** chip-tool must use the fabric where **it** commissioned the device. The node ID (`0x18`) can be correct while the compressed fabric ID in mDNS is wrong — always confirm with `basicinformation read` before OTA.

---

## Home Assistant OTA (in progress)

Production-like OTA through Home Assistant (DCL TestNet + Update entity) is documented step-by-step in **[HAOTA.md](HAOTA.md)** while it is being validated.

**Recommended order:** complete [Local Matter OTA](#local-matter-ota-chip-tool--ota-provider) first (validates firmware, partitions, and OTA slots), then follow HAOTA.md for the HA + DCL path. Once HA OTA testing succeeds, the HAOTA content will be merged into this file.

---

## Other hardware checks (TODO)

Short manual checks to add here later:

- [ ] Commission via BLE-WiFi / on-network (per app)
- [ ] Read cluster attributes (binary sensor state, button events, BME680)
- [ ] Factory reset (GPIO9 long-press) clears fabrics and re-opens commissioning
- [ ] Battery / Power Source attributes (`cosmos_battery`)

Track progress in [POLISH_PLAN.md](POLISH_PLAN.md).
