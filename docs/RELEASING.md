# Releasing firmware apps

Each ESP-IDF app sets version in its root `CMakeLists.txt`:

| Variable | Role |
|----------|------|
| `PROJECT_VER` | Human string **`MAJOR.MINOR.PATCH`** (e.g. `0.1.0`, `1.1.0`) — Matter / OTA software version string |
| `PROJECT_VER_NUMBER` | Monotonic integer — must match `CONFIG_DEVICE_SOFTWARE_VERSION_NUMBER` in that app’s `sdkconfig.defaults` |

Always keep three numeric components. Do **not** use `1.0` or `1.1`; use `1.0.0` / `1.1.0`.

## Current app versions

| App | `PROJECT_VER` | `PROJECT_VER_NUMBER` |
|-----|---------------|----------------------|
| iotDoorSensor | `1.1.0` | 2 |
| iotDualModeBtn | `1.0.0` | 1 |
| iotEnvironmentalSensor | `1.0.0` | 1 |
| iotBedsideLamp | `0.1.0` | 1 |
| iotDoorIntercom | `0.1.0` | 1 |

## When to bump what

Treat `PROJECT_VER` like [SemVer](https://semver.org/) for **product behavior** (not C ABI). On **every** tagged release that ships to devices, also increment `PROJECT_VER_NUMBER` by at least 1 (Matter OTA requires a higher software version number).

### `PATCH` — `x.y.Z` → `x.y.(Z+1)` (e.g. `0.1.0` → `0.1.1`)

Ship when the change is a fix or small internal tweak with **no intentional behavior/API/hardware contract change** for users or HA:

- Bug fixes (crash, wrong LED, stream stall, false PIR)
- Performance / logging / cert regen without new endpoints
- Docs-only does **not** need a firmware version bump
- HA YAML-only changes do **not** bump firmware `PROJECT_VER`

### `MINOR` — `x.Y.z` → `x.(Y+1).0` (e.g. `0.1.0` → `0.2.0`)

Ship when you add **backward-compatible** capability:

- New optional Matter attribute / helper that old HA packages can ignore
- New non-breaking feature (extra LED effect, optional sensor)
- Carrier-compatible GPIO still matches [HARDWARE.md](HARDWARE.md) for that SKU revision

Reset `PATCH` to `0` when bumping `MINOR`.

### `MAJOR` — `X.y.z` → `(X+1).0.0` (e.g. `0.1.0` → `1.0.0`, or `1.1.0` → `2.0.0`)

Ship when something **breaks or redefines** the product contract:

- GPIO / pinout change that needs a new PCB revision
- Matter endpoint set or device type change that requires re-commission or HA entity remapping
- Removing or renaming a user-visible behavior (stream gate semantics, siren latch model, etc.)
- First “gift / production Beta” cut from a `0.x` line → **`1.0.0`**

Reset `MINOR` and `PATCH` to `0` when bumping `MAJOR`.

### Pre-`1.0.0` (`0.x.y`)

`0.MINOR.PATCH` means **still evolving** (MVP / Beta). Prefer:

- Frequent `0.x.y` patch/minor bumps while iterating
- Jump to **`1.0.0`** when the SKU is ready for external gift installs with a frozen pinout + Matter model for that hardware rev

Breaking changes **are allowed** in `0.x` without a major bump, but still document them and always increment `PROJECT_VER_NUMBER`.

### `PROJECT_VER_NUMBER` (always)

| Rule | Detail |
|------|--------|
| Monotonic | Every OTA-able build that may reach a device must have a **higher** number than the last flashed build |
| Sync | `PROJECT_VER_NUMBER` == `CONFIG_DEVICE_SOFTWARE_VERSION_NUMBER` |
| Independent of SemVer math | You may go `0.1.0` (n=1) → `0.1.1` (n=2) → `1.0.0` (n=3); never reuse a number |

Field tip: flash units at *N*, publish OTA at *N+1* (see [BUILD.md](BUILD.md#ota-images)).

## Tag naming

One tag per app release (not a single monorepo version):

```text
iotDoorSensor-v1.1.0
iotDualModeBtn-v1.0.0
iotBedsideLamp-v0.1.0
iotDoorIntercom-v0.1.0
iotEnvironmentalSensor-v1.0.0
```

Pattern: `<app-dir>-v<PROJECT_VER>` with full `x.y.z`.

## Checklist before tagging

1. Decide PATCH / MINOR / MAJOR using the rules above.
2. Bump `PROJECT_VER` and `PROJECT_VER_NUMBER` in the app `CMakeLists.txt`, and `CONFIG_DEVICE_SOFTWARE_VERSION_NUMBER` in that app’s `sdkconfig.defaults`.
3. Update the version table in this file.
4. `idf.py build` succeeds; OTA artifact `<app>-ota.bin` under `build/` when `CHIP_OTA_IMAGE_BUILD=y`.
5. Update [HARDWARE.md](HARDWARE.md) / [MANUFACTURING.md](MANUFACTURING.md) if pins or mfg steps changed.
6. Annotated tag from `main` (or the release branch):

```bash
git tag -a iotDoorIntercom-v0.1.0 -m "iotDoorIntercom PROJECT_VER 0.1.0"
git push origin iotDoorIntercom-v0.1.0
```

7. Field OTA rollout: [cosmos-ha-field](https://github.com/CosmosKiller/cosmos-ha-field).

Do **not** force-push tags that were already used for flashed units.
