/**
 * @file csi_presence_task.cpp
 * @brief Map ESPectre motion callbacks onto the Matter occupancy cluster.
 *
 * ESPectre (francescopace/espectre) is GPL-3.0-only. Linking it makes the
 * combined firmware image a GPLv3 work when that image is distributed.
 */

#include <csi_presence_task.h>

#include <cstdarg>

#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include <clusters/occupancy_sensing/integration.h>
#include <espectre_sdk.h>

static const char *TAG = "csi_presence";

using namespace esp_matter;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

static constexpr EventBits_t CSI_PRESENCE_STA_IP_BIT = BIT0;

static uint16_t s_endpoint_id = 0;
static EventGroupHandle_t s_sta_events = nullptr;
static espectre::RuntimeFrontendController s_runtime;

static esp_log_level_t csi_presence_idf_level(espectre::LogLevel level)
{
    switch (level) {
    case espectre::LogLevel::ERROR:
        return ESP_LOG_ERROR;
    case espectre::LogLevel::WARNING:
        return ESP_LOG_WARN;
    case espectre::LogLevel::INFO:
        return ESP_LOG_INFO;
    case espectre::LogLevel::DEBUG:
        return ESP_LOG_DEBUG;
    case espectre::LogLevel::VERBOSE:
        return ESP_LOG_VERBOSE;
    }
    return ESP_LOG_NONE;
}

static bool csi_presence_log_enabled(void *ctx, espectre::LogLevel level, const char *tag)
{
    (void)ctx;
    return tag != nullptr && csi_presence_idf_level(level) <= esp_log_level_get(tag);
}

static void csi_presence_log_write(void *ctx, espectre::LogLevel level, const char *tag, int line, const char *format,
                                   va_list args)
{
    (void)ctx;
    (void)line;
    esp_log_va(ESP_LOG_CONFIG_INIT(csi_presence_idf_level(level) | ESP_LOG_CONFIGS_DEFAULT), tag, format, args);
}

static void csi_presence_apply(uint16_t endpoint_id, bool occupied)
{
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, occupied]() {
        auto *cluster = OccupancySensing::FindClusterOnEndpoint(endpoint_id);
        if (cluster == nullptr) {
            ESP_LOGE(TAG, "Occupancy cluster missing on ep=%u", endpoint_id);
            return;
        }
        cluster->SetOccupancy(occupied);
        ESP_LOGI(TAG, "Occupancy ep=%u occupied=%d", endpoint_id, occupied);
    });
}

namespace
{

    class csi_presence_listener : public espectre::IRuntimeListener
    {
    public:
        void on_motion_state_changed(const espectre::RuntimeSnapshot &snapshot) override
        {
            /* Not-ready transitions (link drop) keep the last published occupancy. */
            if (!snapshot.ready_to_publish || s_endpoint_id == 0) {
                return;
            }
            const bool occupied = snapshot.motion_state == espectre::MotionState::MOTION;
            csi_presence_apply(s_endpoint_id, occupied);
        }

        void on_sensing_readiness_changed(const espectre::RuntimeSnapshot &snapshot) override
        {
            ESP_LOGI(TAG, "Sensing %s", snapshot.ready_to_publish ? "ready" : "waiting for calibration");
        }

        void on_runtime_fault(const char *message) override
        {
            ESP_LOGE(TAG, "Runtime fault: %s", message != nullptr ? message : "(null)");
        }
    };

    csi_presence_listener s_listener;

} // namespace

static bool csi_presence_sta_has_ipv4(void)
{
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == nullptr) {
        return false;
    }
    esp_netif_ip_info_t info = {};
    if (esp_netif_get_ip_info(netif, &info) != ESP_OK) {
        return false;
    }
    return info.ip.addr != 0;
}

static void csi_presence_ip_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_base;
    (void)event_id;
    (void)event_data;
    if (s_sta_events != nullptr) {
        xEventGroupSetBits(s_sta_events, CSI_PRESENCE_STA_IP_BIT);
    }
}

static void csi_presence_worker(void *arg)
{
    (void)arg;

    if (!csi_presence_sta_has_ipv4()) {
        ESP_LOGI(TAG, "Waiting for Wi-Fi station IPv4 before CSI setup");
        xEventGroupWaitBits(s_sta_events, CSI_PRESENCE_STA_IP_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    }

    if (!espectre::set_log_sink({nullptr, csi_presence_log_enabled, csi_presence_log_write})) {
        ESP_LOGE(TAG, "ESPectre log sink rejected");
        vTaskDelete(nullptr);
        return;
    }

    ESP_LOGI(TAG, "ESPectre SDK %s", ESPECTRE_SDK_VERSION_STRING);

    espectre::RuntimeConfig config = espectre::make_runtime_sensing_config_from_kconfig();
    s_runtime.set_config(config);

    while (!s_runtime.setup(&s_listener)) {
        ESP_LOGE(TAG, "ESPectre setup failed; retrying");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    ESP_LOGI(TAG, "CSI runtime started (Matter station stays owned by esp-matter)");

    while (true) {
        s_runtime.loop();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

esp_err_t csi_presence_endpoint_create(esp_matter::node_t *node)
{
    if (node == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    occupancy_sensor::config_t presence_cfg;
    presence_cfg.occupancy_sensing.feature_flags = chip::to_underlying(OccupancySensing::Feature::kRFSensing);
    endpoint_t *presence_ep = occupancy_sensor::create(node, &presence_cfg, ENDPOINT_FLAG_NONE, nullptr);
    if (presence_ep == nullptr) {
        ESP_LOGE(TAG, "Failed to create occupancy endpoint");
        return ESP_FAIL;
    }

    s_endpoint_id = endpoint::get_id(presence_ep);
    ESP_LOGI(TAG, "Occupancy (CSI / RF sensing) endpoint ID: %u", s_endpoint_id);
    return ESP_OK;
}

esp_err_t csi_presence_task_start(void)
{
    if (s_endpoint_id == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_sta_events != nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    s_sta_events = xEventGroupCreate();
    if (s_sta_events == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, csi_presence_ip_handler, nullptr);
    if (err != ESP_OK) {
        return err;
    }

    if (xTaskCreate(csi_presence_worker, "csi_presence", 8192, nullptr, 5, nullptr) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
