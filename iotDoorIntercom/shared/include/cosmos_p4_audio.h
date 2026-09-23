/**
 * @file cosmos_p4_audio.h
 * @brief P4 Matter→ES8311 audio levels via BRIDGE_CMD_SET_AUDIO.
 */

#pragma once

#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register BRIDGE_CMD_SET_AUDIO and seed default mic/speaker levels.
 *
 * Call after bridge_cmd_init(). Levels are applied when media_stream opens
 * the ES8311 codecs (first Live View), or immediately if already open.
 */
esp_err_t cosmos_p4_audio_init(void);

#ifdef __cplusplus
}
#endif
