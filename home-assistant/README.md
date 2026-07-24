# Home Assistant — SKU packages (firmware monorepo)

This folder holds **device/SKU-specific** HA YAML that ships with firmware documentation.

| File | Purpose |
|------|---------|
| [packages/cosmos_binary_sensor.yaml](packages/cosmos_binary_sensor.yaml) | Low-battery notify + recovery latch for `iotBasicBinarySensor` |

Copy into HA OS: `/config/packages/` and enable `packages: !include_dir_named packages` in `configuration.yaml`.

**Commissioning, Pi field OTA, and fleet automation** live in the separate repo **[cosmos-ha-field](https://github.com/CosmosKiller/cosmos-ha-field)** (install on Pi: [DEPLOY_TO_PI.md](https://github.com/CosmosKiller/cosmos-ha-field/blob/main/DEPLOY_TO_PI.md)).
