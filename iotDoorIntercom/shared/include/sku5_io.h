/**
 * @file sku5_io.h
 * @brief Compatibility name for cosmos_p4_security_io_init (streaming_only call site).
 */
#pragma once

#include "cosmos_p4_security_io.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t sku5_io_init(void);

#ifdef __cplusplus
}
#endif
