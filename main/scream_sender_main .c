#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "secrets.h"
#include "spdif_in.h"
#include <string.h>

#define SAMPLE_RATE 48000
#define CHUNK_SIZE 1152
#define HEADER_SIZE 5
#define PACKET_SIZE (CHUNK_SIZE + HEADER_SIZE)

static const char *TAG = "ESP32SPDIFScreamSender";

const char header[] = {1, 16, 2, 0, 0}; // Stereo 16 bit 48KHz default layout
char data_out[PACKET_SIZE];
char data_in[CHUNK_SIZE * 8];
int data_in_head = 0;

int sock = -1;
struct sockaddr_in dest_addr;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

static void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
}

void spdif_task(void *pvParameters)
{
    while (1) {
        data_in_head += spdif_read(data_in + data_in_head, CHUNK_SIZE);
        while (data_in_head >= CHUNK_SIZE) {
            memcpy(data_out + HEADER_SIZE, data_in, CHUNK_SIZE);
            sendto(sock, data_out, PACKET_SIZE, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            data_in_head -= CHUNK_SIZE;
            for (int i=0;i<data_in_head;i++)
                data_in[i] = data_in[i + CHUNK_SIZE];
        }
    }
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Copy header to data_out
    memcpy(data_out, header, HEADER_SIZE);

    // Initialize and connect to Wi-Fi
    wifi_init_sta();

    // Initialize SPDIF
    spdif_init(SAMPLE_RATE);

    // Initialize UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return;
    }
    
    dest_addr.sin_addr.s_addr = inet_addr(SERVER);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);

    // Create SPDIF reading task
    xTaskCreate(spdif_task, "spdif_task", 4096, NULL, 5, NULL);

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
