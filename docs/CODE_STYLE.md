# Cosmos code style

Conventions for firmware in this monorepo (`main/`, `tasks/`, and future `components/`). They come from the legacy CosmosIoT platform guide and apply to **project code we write**.

When calling ESP-IDF, esp-matter, or CHIP APIs, follow upstream naming (`esp_err_t`, `kTimeoutSeconds`, etc.) at the boundary; use Cosmos rules inside our modules.

## Formatting (clang-format)

Style is **LLVM-based** with **Linux** brace breaks (opening brace on the same line except function definitions), **4-space** indents, no tabs, and aligned consecutive macros.

A repo-root [`.clang-format`](../.clang-format) encodes this. Format before commit:

```bash
clang-format -i path/to/file.cpp
```

### VS Code / Cursor

1. Install the **C/C++** extension.
2. Set **Clang_format_fallback Style** to (or rely on the repo `.clang-format` with **C_Cpp.clang_format_style**: `file`):

```
{ BasedOnStyle: LLVM, UseTab: Never, IndentWidth: 4, TabWidth: 4, BreakBeforeBraces: Linux, AllowShortIfStatementsOnASingleLine: false, IndentCaseLabels: false, ColumnLimit: 0, AccessModifierOffset: -4, NamespaceIndentation: All, FixNamespaceComments: false, AlignConsecutiveMacros: true }
```

3. Enable **Format On Save**, **Format On Paste**, and **Format On Type** for C/C++.

## Naming

| Kind | Convention | Example |
|------|------------|---------|
| Variables, functions | `snake_case` | `binary_sensor_task_init` |
| Pointer variables | `camelCase` with leading `p` | `cosmos_devices_t *pSocket` |
| `typedef struct` | `snake_case` + `_t` suffix | `binary_sensor_ctx_t` |
| `typedef enum` | `snake_case` + `_e` suffix | `evt_source_e` |
| Enum members | `UPPER_SNAKE_CASE` | `EVT_SOURCE_ALARM` |
| Macros / `#define` | `UPPER_SNAKE_CASE` | `SINGLE_PRESS_LED_PIN` |

Matter-specific globals already in the tree (e.g. `iot_button_endpoint_id`) can stay; use these rules for **new** symbols.

## Comments

- **Single line:** `//`
- **Multi-line:** `/* ... */` with a `*` prefix on each line, lines aligned:

```c
/*
 * This is a multi-line comment.
 * Keep line lengths even where practical.
 */
```

## Documentation (Doxygen)

Public APIs in `tasks/*.h` (and shared headers) should use Doxygen blocks.

**Functions / files:** `/** ... */` with `@brief`, `@param`, `@return`, `@note` as needed.

```c
/**
 * @brief Initialize the binary sensor GPIO and ISR wiring.
 *
 * @param config Driver config including endpoint id and callback.
 * @return ESP_OK on success, or an ESP_ERR_* code.
 */
esp_err_t binary_sensor_task_init(binary_sensor_config_t *config);
```

**Struct / enum / `#define` members:** trailing `/*!< description */` on the same line when short:

```c
typedef struct {
    const char sn[15]; /*!< Device serial number */
    int state;         /*!< Initial line level (0 low, 1 high) */
} cosmos_devices_t;

#define NO_OF_SAMPLES 64 /*!< ADC multisample count */
```

File headers should include `@file`, `@brief`, and copyright where you already use them in `main.cpp`.

## Layout (project structure)

- **`main/`** — implementations (`*.cpp`, `*.c`)
- **`tasks/`** — public headers (`*.h`)

See [REPO_LAYOUT.md](REPO_LAYOUT.md).

## Reference sample

The legacy guide included a full header example (`cosmos_pump_control`, enums, struct members with `/*!< */`). Use that as a template for new `tasks/*.h` files; a copy of the original write-up is in [`oldReadme.md`](../oldReadme.md) at the repo root until you remove it.

## Matter / threading

Cosmos style does not replace Matter rules: schedule attribute updates on the CHIP system layer from GPIO or button callbacks — see [CONTRIBUTING.md](CONTRIBUTING.md).
