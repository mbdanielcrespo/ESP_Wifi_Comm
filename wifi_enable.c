#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "driver/gpio.h"

/* Include configuration settings */
#include "wifi_config.h"

#define WIFI_SUCCESS BIT0
#define WIFI_FAILURE BIT1

static const char *TAG = LOG_TAG_NETWORK;

static bool server_found = false; // Flag to stop scanning once the server is found

/** GLOBALS **/
static EventGroupHandle_t wifi_event_group;
static int s_retry_num = 0;

/** WIFI EVENT HANDLERS **/
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "Connecting to AP...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < MAX_WIFI_RETRIES)
        {
            ESP_LOGI(TAG, "Reconnecting to AP...");
            esp_wifi_connect();
            s_retry_num++;
        }
        else
            xEventGroupSetBits(wifi_event_group, WIFI_FAILURE);
    }
}

static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "STA IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_SUCCESS);
    }
}

/** CONNECT TO WIFI **/
void connect_wifi()
{
    ESP_LOGI(TAG, "Initializing WiFi...");

    // Initialize the TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create the default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create the default Wi-Fi station interface
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    // Initialize Wi-Fi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Create an event group for Wi-Fi events
    wifi_event_group = xEventGroupCreate();

    // Declare handler instances
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    // Register event handlers for Wi-Fi and IP events
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &ip_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    // Configure Wi-Fi connection settings using configuration macros
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    
    // Copy SSID and password from configuration
    strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, WIFI_PASSWORD, sizeof(wifi_config.sta.password));

    // Stop DHCP client to assign a static IP
    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(sta_netif));

    // Set static IP configuration from config macros
    esp_netif_ip_info_t ip_info = {0};
    ESP_ERROR_CHECK(esp_netif_str_to_ip4(ESP_STATIC_IP, &ip_info.ip));
    ESP_ERROR_CHECK(esp_netif_str_to_ip4(ESP_NETMASK, &ip_info.netmask));
    ESP_ERROR_CHECK(esp_netif_str_to_ip4(ESP_GATEWAY, &ip_info.gw));
    ESP_ERROR_CHECK(esp_netif_set_ip_info(sta_netif, &ip_info));

    // Set Wi-Fi mode to station
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Set Wi-Fi configuration
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Start Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Waiting for WiFi connection to %s...", WIFI_SSID);

    // Wait for connection event
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_SUCCESS | WIFI_FAILURE,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    // Check connection status
    if (bits & WIFI_SUCCESS)
    {
        ESP_LOGI(TAG, "WiFi connected successfully.");
    }
    else if (bits & WIFI_FAILURE)
    {
        ESP_LOGE(TAG, "WiFi connection failed after %d retries.", MAX_WIFI_RETRIES);
        esp_restart();
    }

    // Unregister event handlers and delete event group
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(wifi_event_group);
}


/** QUICK PING AN IP ADDRESS **/
bool quick_ping_ip(const char *ip)
{
    struct sockaddr_in serverInfo = {0};
    int sock;

    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr.s_addr = inet_addr(ip);
    serverInfo.sin_port = htons(SERVER_PORT);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        ESP_LOGE(TAG, "Socket creation failed for %s", ip);
        return false;
    }

    // Set socket to non-blocking mode
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    connect(sock, (struct sockaddr *)&serverInfo, sizeof(serverInfo));

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = PING_TIMEOUT_MS * 1000; // Convert to microseconds
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(sock, &writefds);

    int result = select(sock + 1, NULL, &writefds, NULL, &timeout);
    close(sock);

    return result > 0 && FD_ISSET(sock, &writefds);
}

/** TCP CONNECTION TASK **/
void tcp_connection_task(void *param)
{
    char *ip = (char *)param;
    struct sockaddr_in serverInfo = {0};
    int sock;
    char buffer[128];

    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr.s_addr = inet_addr(ip);
    serverInfo.sin_port = htons(SERVER_PORT);

    ESP_LOGI(TAG, "Attempting TCP connection to %s:%d...", ip, SERVER_PORT);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        ESP_LOGE(TAG, "Failed to create socket for %s", ip);
        free(ip);
        vTaskDelete(NULL);
        return;
    }

    if (connect(sock, (struct sockaddr *)&serverInfo, sizeof(serverInfo)) == 0)
    {
        ESP_LOGI(TAG, "Connected to server at %s", ip);
        server_found = true; // Mark server as found

        while (1)
        {
            int len = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (len > 0)
            {
                buffer[len] = '\0';
                ESP_LOGI(TAG, "Received: %s", buffer);

                if (strcmp(buffer, SERVER_ID) == 0)
                {
                    ESP_LOGI(TAG, "Desired server found at %s!", ip);
                }
                else if (strcmp(buffer, GATE_CMD) == 0)
                {
                    ESP_LOGI(TAG, "GATE command received. Toggling relay...");

                    gpio_set_level(GPIO_GATE_PIN, 1);
                    vTaskDelay(pdMS_TO_TICKS(GATE_OPEN_DURATION)); // Keep the gate open for configured duration
                    gpio_set_level(GPIO_GATE_PIN, 0);
                    ESP_LOGI(TAG, "Gate toggled off.");
                }
            }
            else if (len == 0)
            {
                ESP_LOGI(TAG, "Connection closed by server at %s", ip);
                break;
            }
            else
            {
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    ESP_LOGE(TAG, "Error receiving data from %s: errno %d", ip, errno);
                    break;
                }
            }
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to connect to %s", ip);
    }

    close(sock);
    server_found = false; // Mark server as not found
    free(ip);
    vTaskDelete(NULL);
}

/** SCAN NETWORK **/
void scan_network(void)
{
    char ip[16];

    while (1)
    {
        if (!server_found)
        {
            for (int i = SCAN_START_IP; i >= SCAN_END_IP; i--)
            {
                if (server_found)
                {
                    ESP_LOGI(TAG, "Server already found. Stopping scan.");
                    break;
                }

                snprintf(ip, sizeof(ip), "%s%d", IP_PREFIX, i);

                if (quick_ping_ip(ip))
                {
                    ESP_LOGI(TAG, "IP %s is live, creating TCP connection task...", ip);
                    xTaskCreate(tcp_connection_task, "tcp_connection_task", TASK_STACK_SIZE, strdup(ip), 5, NULL);
                }
                else
                {
                    ESP_LOGI(TAG, "IP %s is not live.", ip);
                }

                vTaskDelay(pdMS_TO_TICKS(SCAN_RETRY_DELAY_MS)); // Delay between pings
            }

            if (!server_found)
            {
                ESP_LOGI(TAG, "Reached end of IP range. Restarting scan...");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(SCAN_INTERVAL_MS)); // Delay before rescanning
    }
}

/** GPIO INITIALIZATION **/
void gpio_init()
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << GPIO_GATE_PIN),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&io_conf);

    gpio_set_level(GPIO_GATE_PIN, 0); // Ensure gate is initially off
    ESP_LOGI(TAG, "GPIO %d initialized as output for gate control.", GPIO_GATE_PIN);
}

void app_main()
{
    ESP_LOGI(TAG, "Starting WiFi gate controller application...");
    ESP_LOGI(TAG, "Using configuration: SSID=%s, Static IP=%s", WIFI_SSID, ESP_STATIC_IP);

    nvs_flash_init();
    gpio_init();
    connect_wifi();
    scan_network();

    // Wait for commands indefinitely
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Keep the main task alive
    }
}
