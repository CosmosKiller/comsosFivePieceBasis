# Repository layout — current and target

## Current (today)

```
cosmosFivePieceBasis/
├── README.md
├── LICENSE
├── docs/                          # shared docs (BUILD, HARDWARE, POLISH_PLAN, …)
├── iotBasicBinarySensor/
│   ├── main/          (.cpp/.c sources + CMakeLists)
│   ├── tasks/         (headers only)
│   ├── build/         (generated, gitignored)
│   ├── managed_components/
│   ├── mfg_tool_scripts/  (local, gitignored)
│   └── To-Do.MD       (local checklist, gitignored)
├── iotDualModeBtn/
│   └── (same pattern; sdkconfig.defaults* for C6)
└── iotEnvironmentalSensor/
    └── (same pattern)
```

**Layout (Option B — consistent across all apps)**

- **`tasks/`** — public headers (`*.h`) only
- **`main/`** — implementations (`*.cpp`, `*.c`), `main.cpp`, `CMakeLists.txt`, `Kconfig.projbuild` (ESP-IDF convention)
- Register sources in `main/CMakeLists.txt` with `INCLUDE_DIRS "." "../tasks"`

**Build defaults**

- Each app: `sdkconfig.defaults` (+ dual-mode `sdkconfig.defaults.c6_*` variants)
- Shared baseline: [docs/sdkconfig.defaults.matter-base](docs/sdkconfig.defaults.matter-base)

**Generated locally (not in git)**

- `sdkconfig` / `sdkconfig.old` — created by `idf.py set-target` from `sdkconfig.defaults`

## Target (professional baseline)

No need to rename existing `iot*` folders immediately — avoids breaking paths and muscle memory. Evolve toward:

```
cosmosFivePieceBasis/
├── README.md
├── LICENSE
├── .github/
│   └── workflows/
│       └── build.yml              # matrix: each firmware app × target
├── docs/
│   ├── BUILD.md
│   ├── HARDWARE.md
│   ├── REPO_LAYOUT.md
│   ├── POLISH_PLAN.md
│   └── CONTRIBUTING.md            # optional, when collaborators appear
├── components/                    # ESP-IDF shared components
│   ├── cosmos_matter_common/      # factory reset + Matter event helpers
│   └── cosmos_battery/            # ADC sampling + Matter Power Source endpoint
├── iotBasicBinarySensor/          # thin apps: main + device-specific tasks only
├── iotDualModeBtn/
├── iotEnvironmentalSensor/
└── tools/                         # optional
    └── scripts/
        └── build_all.sh
```

### Per-app layout (target)

Pick **one** convention and apply everywhere:

**Option A — headers co-located (simplest)**

```
iotDualModeBtn/
├── main/
│   ├── main.cpp
│   ├── matter_task.cpp
│   ├── matter_task.h
│   └── ...
└── CMakeLists.txt
```

**Option B — keep `tasks/` for public API (current direction)**

```
iotDualModeBtn/
├── main/          # all .cpp / .c implementations
├── tasks/         # all .h consumed by main and other components
└── CMakeLists.txt # INCLUDE_DIRS "." "../tasks"
```

Recommendation: **Option B** — already used; finish moving any stray headers/sources so every task has `tasks/foo.h` + `main/foo.cpp`.

### `components/cosmos_matter_common`

Extract only what is **identical** across apps first:

- Factory reset button handling
- Shared `app_event_cb` fabric / commissioning window logic
- NVS init helper (if duplicated)

Keep in each app:

- Endpoint and cluster creation in `main.cpp`
- Device-specific tasks and GPIO maps

Register in each app root `CMakeLists.txt`:

```cmake
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../components"
    ...
)
```

### Git / generated files

| Track in git | Do not track |
|--------------|--------------|
| `sdkconfig.defaults`, `sdkconfig.defaults.*`, `dependencies.lock` | `sdkconfig`, `sdkconfig.old` |
| `partitions.csv`, source, `docs/` | `build/`, `managed_components/`, `log.txt`, `out/` |

### CI

Implemented in [`.github/workflows/build.yml`](../.github/workflows/build.yml): matrix build of all three apps using the `espressif/esp-matter:latest` container (`idf.py set-target` + `idf.py build` from `sdkconfig.defaults` only).
