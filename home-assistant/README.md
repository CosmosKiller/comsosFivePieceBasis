# Home Assistant — SKU packages (firmware monorepo)

Device/SKU HA YAML that ships with firmware documentation. **Canonical** location is here; [cosmos-ha-field](https://github.com/CosmosKiller/cosmos-ha-field) keeps deploy copies for the Pi.

| Path | Purpose |
|------|---------|
| [packages/cosmos_door_sensor.yaml](packages/cosmos_door_sensor.yaml) | Low-battery notify + latch — `iotDoorSensor` |
| [packages/cosmos_door_intercom.yaml](packages/cosmos_door_intercom.yaml) | Helpers + doorbell/PIR/tamper automations — `iotDoorIntercom` |
| [packages/cosmos_security.yaml](packages/cosmos_security.yaml) | Intrusion alarm for door/window, intercom tamper, or any trip (strobe + sirens) |
| [lovelace/cosmos_door_intercom.yaml](lovelace/cosmos_door_intercom.yaml) | Door-station dashboard view (stock cards) |
| [secrets.yaml.example](secrets.yaml.example) | Optional stream URL note (camera is UI-configured) |

## Install packages

1. Copy desired files → HA OS `/config/packages/`
2. In `configuration.yaml`:
   ```yaml
   homeassistant:
     packages: !include_dir_named packages
   ```
3. **Developer tools → Check configuration** → restart HA.
4. Confirm helpers exist under States, e.g. `input_boolean.cosmos_door_intercom_auto_stream`.
5. Replace every `# TODO` entity id after Matter commissioning.

## Add intercom MJPEG camera (UI — required)

YAML `camera: platform: mjpeg` is **not** supported in current HA. Add the stream in the UI:

1. Enable the Matter **stream gate** switch (so `/stream` answers).
2. **Settings → Devices & services → Add integration → MJPEG IP Camera**
3. MJPEG URL: `https://<device-ip>/stream`  
   Verify SSL: **off** (self-signed Beta cert)  
   Name: **Cosmos Door Intercom** (entity becomes `camera.cosmos_door_intercom`)
4. If the entity id differs, rename it or update Lovelace to match.

## Install door Lovelace view

1. Open the dashboard → ⋮ → **Edit** → **Raw configuration editor** (or YAML mode).
2. Under `views:`, add the contents of [lovelace/cosmos_door_intercom.yaml](lovelace/cosmos_door_intercom.yaml) as **one list item** (keep its `title` / `path` / `cards`).
3. Fix TODO entity ids to match the package.
4. Save. Open path **`/lovelace/cosmos-door`** (or your dashboard URL + `/cosmos-door`).

No HACS / custom JS required — conditional `picture-entity` + buttons. A branded custom card can wait until gift UX needs it.

## Security groups (`cosmos_security` package)

| Group | Entity | Purpose |
|-------|--------|---------|
| Security lights | `light.security_system_lights` | Red strobe during intrusion (create in UI or YAML — not in package) |
| Security sirens | `switch.security_system_sirens` | **Switch group** in [packages/cosmos_security.yaml](packages/cosmos_security.yaml) — all Matter siren switches |

After commissioning each unit, add its `switch.*_siren` to the **Security System Sirens** group (edit the package list, or add members in **Settings → Devices → Helpers** if you recreate the group in UI). Non-Cosmos switches (MQTT, template, etc.) can join the same group so `cosmos_security` and voice assistants target one entity.

**Multi door sensor:** one `switch.*_siren` line per unit; arm with a separate switch group on `switch.*_arm`.

## Entity checklist (intercom)

After commissioning, rename the Matter device to **Cosmos Door Intercom** (optional) and note:

| Role | Typical domain | Used as |
|------|----------------|---------|
| Stream gate (OnOff plug) | `switch.*` | Enable / disable HTTPS `/stream` |
| PIR occupancy | `binary_sensor.*` | Auto stream + dashboard status |
| Doorbell (`generic_switch` 0x000F) | `event.*` | Ring notify + auto stream (HA-first; not Matter Doorbell 0x0148 yet) |
| Tamper (contact / Boolean State) | `binary_sensor.*` | Mount open = on; notify + stream + latched siren |
| Siren clear (mounted OnOff) | `switch.*` | On = sounding / test; **Off = silence** (remount does not clear) |
| MJPEG camera | `camera.cosmos_door_intercom` | Created via UI (MJPEG IP Camera) |

## Entity checklist (door sensor)

After commissioning, rename the Matter device to **Cosmos Door Sensor** (optional). Firmware **Fixed Labels** default endpoint names to **Door contact**, **Arm / disarm**, **Panic alarm**, and **Siren**:

| Role | Typical domain | Default Matter name | Notes |
|------|----------------|---------------------|-------|
| Contact (Boolean State) | `binary_sensor.*` | Door contact | Reed open = on |
| Arm/disarm (OnOff) | `switch.*` | Arm / disarm | ON = arm; **OFF = disarm + clear panic/siren + silence** |
| Panic indicator (Boolean State) | `binary_sensor.*` | Panic alarm | Read-only; ON = intrusion; off when reed closes |
| Siren (mounted OnOff) | `switch.*` | Siren | ON = buzzer (HA group, Alexa, any automation); **Off = silence** |
| Battery (Power Source) | `sensor.*` | (varies) | Low-battery notify in package |

**Multi-sensor pattern:** arm all units via an HA `switch` group; only opened contacts trip. Turn **any** unit's siren **On** from an automation to sound that buzzer — works with non-Cosmos devices too if they expose a controllable siren/switch in HA.

## Audio (later)

XIAO ESP32-S3 Sense has a PDM mic, but the current firmware path is **video-only MJPEG**. Two-way talk needs a later audio/WebRTC slice — not required for HA gift Beta with doorbell + live view.

## Sync deploy copy → cosmos-ha-field

Canonical YAML lives here. Push packages + Lovelace to the ha-field clone:

```bash
rsync -av --delete \
  /home/cosmos/myProjects/pioIdfTest/cosmosFivePieceBasis/home-assistant/packages/ \
  /home/cosmos/uHome/myProjects/cosmos-ha-field/packages/ && \
rsync -av --delete \
  /home/cosmos/myProjects/pioIdfTest/cosmosFivePieceBasis/home-assistant/lovelace/ \
  /home/cosmos/uHome/myProjects/cosmos-ha-field/lovelace/
```

Adjust the ha-field path if your clone lives elsewhere. `--delete` makes the destination match this tree (extra files in those folders are removed).

## Field / Pi

Commissioning, Pi OTA, and fleet docs: **[cosmos-ha-field](https://github.com/CosmosKiller/cosmos-ha-field)**.
