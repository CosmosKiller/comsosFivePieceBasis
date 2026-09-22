/*
 * Cosmos SKU5 — security Matter endpoints on C6 (doorbell / PIR / tamper / siren).
 * GPIO lives on P4; events arrive via bridge_cmd BRIDGE_EVT_SECURITY_IO.
 */
#pragma once

#include <esp_err.h>
#include <esp_matter.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t security_endpoints_create(esp_matter::node_t *node);
esp_err_t security_endpoints_register_bridge(void);
void security_endpoints_mark_matter_started(void);
esp_err_t security_endpoints_drive_siren(bool on);
uint16_t security_endpoints_siren_id(void);

#ifdef __cplusplus
}
#endif
