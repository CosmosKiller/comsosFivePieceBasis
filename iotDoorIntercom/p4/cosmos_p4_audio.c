/**
 * @file cosmos_p4_audio.c
 * @brief P4 handler: Matter mic/speaker volume+mute → ES8311 via media_stream.
 */

#include <esp_log.h>

#include <bridge_cmd_defs.h>
#include <media_stream.h>
#include <webrtc_bridge.h>

#include <cosmos_p4_audio.h>

static const char *TAG = "cosmos_p4_audio";

/* Match C6 camera-device kMicrophoneDefaultLevel / kSpeakerDefaultLevel. */
static uint8_t s_mic_vol = 254;
static uint8_t s_spk_vol = 254;
static bool s_mic_mute = false;
static bool s_spk_mute = false;

static void apply_desired(void)
{
    media_stream_set_microphone_control(s_mic_vol, s_mic_mute);
    media_stream_set_speaker_control(s_spk_vol, s_spk_mute);
    ESP_LOGI(TAG, "SET_AUDIO apply mic=%u mute=%u spk=%u mute=%u", s_mic_vol, (unsigned)s_mic_mute, s_spk_vol,
             (unsigned)s_spk_mute);
}

static esp_err_t handle_set_audio(uint32_t cmd_id, const uint8_t *req_data, size_t req_len, uint8_t **resp_data,
                                  size_t *resp_len)
{
    (void)cmd_id;
    *resp_data = NULL;
    *resp_len = 0;

    if (req_data == NULL || req_len < sizeof(bridge_cmd_set_audio_t)) {
        ESP_LOGW(TAG, "SET_AUDIO bad payload len=%u", (unsigned)req_len);
        return ESP_ERR_INVALID_ARG;
    }

    const bridge_cmd_set_audio_t *req = (const bridge_cmd_set_audio_t *)req_data;
    s_mic_vol = req->mic_vol ? req->mic_vol : 1;
    s_spk_vol = req->spk_vol ? req->spk_vol : 1;
    s_mic_mute = req->mic_mute != 0;
    s_spk_mute = req->spk_mute != 0;

    ESP_LOGI(TAG, "SET_AUDIO recv mic=%u mute=%u spk=%u mute=%u", s_mic_vol, (unsigned)s_mic_mute, s_spk_vol,
             (unsigned)s_spk_mute);
    apply_desired();
    return ESP_OK;
}

esp_err_t cosmos_p4_audio_init(void)
{
    esp_err_t err = bridge_cmd_register_handler(BRIDGE_CMD_SET_AUDIO, handle_set_audio);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SET_AUDIO register failed: %s", esp_err_to_name(err));
        return err;
    }

    /* Seed media_stream desired levels before Live View opens codecs. */
    apply_desired();
    ESP_LOGI(TAG, "P4 audio levels ready (SET_AUDIO←C6, defaults mic/spk=254)");
    return ESP_OK;
}
