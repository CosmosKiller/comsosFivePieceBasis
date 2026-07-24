# Home Assistant config snippets

Drop-in packages for Cosmos Matter devices. Not loaded automatically — copy into your HA instance.

## Install

```bash
# On your HA host (adjust path)
cp packages/cosmos_binary_sensor.yaml /config/packages/
```

In `/config/configuration.yaml`:

```yaml
homeassistant:
  packages: !include_dir_named packages
```

Edit entity ids inside the package after commissioning each device. See [docs/HA.md](../docs/HA.md).

## Contents

| File | Purpose |
|------|---------|
| `packages/cosmos_binary_sensor.yaml` | Low-battery notify + recovery latch helpers |
