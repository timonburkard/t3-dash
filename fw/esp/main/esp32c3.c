#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "os/os_mbuf.h"


#define UART_TXD 4
#define UART_RXD 5
#define UART_RTS       (-1)
#define UART_CTS      (-1)
#define UART_PORT_NUM 1
#define UART_BAUD_RATE 115200
#define UART_TASK_STACK_SIZE 3072
#define UART_BUF_SIZE (1024)

#define BLE_DEVICE_NAME "ESP32C3-NimBLE"
#define BLE_GATT_MAX_PAYLOAD 128

static const char *TAG = "ESP32C3_UART_TO_NUCLEO";
static const char *TAG_BLE = "BLE_GATT";

static uint8_t g_own_addr_type;
static uint16_t g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t gatt_chr_val_handle;
static bool g_notify_enabled;
static char gatt_value[BLE_GATT_MAX_PAYLOAD] = "ready";

// Service UUID: c4e7b1a2-0f3d-4f2a-8d89-0d92b48e7d21
static const ble_uuid128_t gatt_svc_uuid = BLE_UUID128_INIT(
    0x21, 0x7d, 0x8e, 0xb4, 0x92, 0x0d, 0x89, 0x8d,
    0x2a, 0x4f, 0x3d, 0x0f, 0xa2, 0xb1, 0xe7, 0xc4);
// Characteristic UUID: c4e7b1a3-0f3d-4f2a-8d89-0d92b48e7d21
static const ble_uuid128_t gatt_chr_uuid = BLE_UUID128_INIT(
    0x21, 0x7d, 0x8e, 0xb4, 0x92, 0x0d, 0x89, 0x8d,
    0x2a, 0x4f, 0x3d, 0x0f, 0xa3, 0xb1, 0xe7, 0xc4);

static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        size_t len = strnlen(gatt_value, sizeof(gatt_value));
        return os_mbuf_append(ctxt->om, gatt_value, len);
    }

    return BLE_ATT_ERR_UNLIKELY;
}

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &gatt_chr_uuid.u,
                .access_cb = gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &gatt_chr_val_handle,
            },
            { 0 }
        },
    },
    { 0 }
};

static void ble_app_advertise(void);

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            g_conn_handle = event->connect.conn_handle;
            ESP_LOGI(TAG_BLE, "Connected; handle=%d", g_conn_handle);
        } else {
            ESP_LOGW(TAG_BLE, "Connect failed; status=%d", event->connect.status);
            ble_app_advertise();
        }
        return 0;
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG_BLE, "Disconnected; reason=%d", event->disconnect.reason);
        g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        g_notify_enabled = false;
        ble_app_advertise();
        return 0;
    case BLE_GAP_EVENT_SUBSCRIBE:
        if (event->subscribe.attr_handle == gatt_chr_val_handle) {
            g_notify_enabled = event->subscribe.cur_notify;
            ESP_LOGI(TAG_BLE, "Notify %s", g_notify_enabled ? "enabled" : "disabled");
        }
        return 0;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ble_app_advertise();
        return 0;
    default:
        return 0;
    }
}

static void ble_on_reset(int reason)
{
    ESP_LOGE(TAG_BLE, "Resetting state; reason=%d", reason);
}

static void ble_app_on_sync(void)
{
    int rc = ble_hs_id_infer_auto(0, &g_own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG_BLE, "ble_hs_id_infer_auto failed: %d", rc);
        return;
    }
    ble_app_advertise();
}

static void ble_app_advertise(void)
{
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.uuids128 = (ble_uuid128_t *)&gatt_svc_uuid;
    fields.num_uuids128 = 1;
    fields.uuids128_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG_BLE, "ble_gap_adv_set_fields failed: %d", rc);
        return;
    }

    struct ble_hs_adv_fields rsp_fields;
    memset(&rsp_fields, 0, sizeof(rsp_fields));
    rsp_fields.name = (uint8_t *)ble_svc_gap_device_name();
    rsp_fields.name_len = strlen(ble_svc_gap_device_name());
    rsp_fields.name_is_complete = 1;

    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        ESP_LOGE(TAG_BLE, "ble_gap_adv_rsp_set_fields failed: %d", rc);
        return;
    }

    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(g_own_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG_BLE, "ble_gap_adv_start failed: %d", rc);
    }
}

void ble_notify_text(const char *text)
{
    if (text == NULL) {
        return;
    }

    size_t len = strnlen(text, BLE_GATT_MAX_PAYLOAD - 1);
    memcpy(gatt_value, text, len);
    gatt_value[len] = '\0';

    if (!g_notify_enabled ||
        g_conn_handle == BLE_HS_CONN_HANDLE_NONE ||
        gatt_chr_val_handle == 0) {
        return;
    }

    struct os_mbuf *om = ble_hs_mbuf_from_flat(gatt_value, len);
    if (om == NULL) {
        ESP_LOGW(TAG_BLE, "Notify: no mbuf");
        return;
    }

    int rc = ble_gatts_notify_custom(g_conn_handle, gatt_chr_val_handle, om);
    if (rc != 0) {
        os_mbuf_free_chain(om);
        ESP_LOGW(TAG_BLE, "Notify failed: %d", rc);
    }
}

static void ble_host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void ble_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_BLE, "nimble_port_init failed: %s", esp_err_to_name(ret));
        return;
    }

    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.sync_cb = ble_app_on_sync;

    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_gap_device_name_set(BLE_DEVICE_NAME);

    int rc_gatt = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc_gatt != 0) {
        ESP_LOGE(TAG_BLE, "ble_gatts_count_cfg failed: %d", rc_gatt);
        return;
    }
    rc_gatt = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc_gatt != 0) {
        ESP_LOGE(TAG_BLE, "ble_gatts_add_svcs failed: %d", rc_gatt);
        return;
    }

    nimble_port_freertos_init(ble_host_task);
}

static void uart_task_esp_to_nucleo(void *arg)
{
    // Configure UART parameters
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    // Install UART driver
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0));
    // Configure UART pins
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TXD, UART_RXD, UART_RTS, UART_CTS));

    uint8_t data[UART_BUF_SIZE];
    while (1) {
        // Read data from UART
        int len = uart_read_bytes(UART_PORT_NUM, data, UART_BUF_SIZE - 1, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            data[len] = '\0'; // Null-terminate the string
            ESP_LOGI(TAG, "Received: %s", (char *)data);
            ble_notify_text((char *)data);
        }
    }
}

void app_main(void)
{
    ble_init();
    xTaskCreate(uart_task_esp_to_nucleo, "uart_task_esp_to_nucleo", UART_TASK_STACK_SIZE, NULL, 10, NULL);

}
