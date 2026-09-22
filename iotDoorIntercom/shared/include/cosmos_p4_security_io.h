/**
 * @file cosmos_p4_security_io.h
 * @brief P4 security I/O bring-up for split-mode media image.
 */

#pragma once

#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Init doorbell/PIR/tamper/siren: EVT + GPIOs + SET_SIREN handler.
 *
 * Call after bridge_cmd_init(). Posts BRIDGE_EVT_SECURITY_IO to C6.
 */
esp_err_t cosmos_p4_security_io_init(void);

#ifdef __cplusplus
}
#endif
