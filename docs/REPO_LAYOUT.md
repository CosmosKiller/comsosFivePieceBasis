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

**Inconsistencies to fix over time**

- Headers in `tasks/`, sources in `main/` (not uniform across all files, e.g. some `.c` only in `main/`)
- Duplicated `matter_task` / `factory_reset_task` per app
- Stale CMake comment (`lilFlowerPal`) in every `main/CMakeLists.txt`
- `sdkconfig` committed per app (works solo; noisy for teams)

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
│   └── cosmos_matter_common/
│       ├── CMakeLists.txt
│       ├── include/
│       │   ├── matter_task.h
│       │   └── factory_reset_task.h
│       └── src/
│           ├── matter_task.cpp      # shared commissioning / fabric callbacks
│           └── factory_reset_task.cpp
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
| `sdkconfig.defaults`, `sdkconfig.defaults.*` | `sdkconfig`, `sdkconfig.old` |
| `partitions.csv`, source, `docs/` | `build/`, `managed_components/` |
| `dependencies.lock` (optional; team policy) | `log.txt`, `out/` |

### CI sketch

```yaml
# .github/workflows/build.yml (future)
strategy:
  matrix:
    app: [iotBasicBinarySensor, iotDualModeBtn, iotEnvironmentalSensor]
    target: [esp32c6]   # add esp32c5 when environmental app is ready
steps:
  - uses: espressif/esp-idf-ci-action@v1
  - run: cd ${{ matrix.app }} && idf.py set-target ${{ matrix.target }} && idf.py build
```

Requires esp-matter in the CI image or a composite setup script — see Phase 4 in [POLISH_PLAN.md](POLISH_PLAN.md).
