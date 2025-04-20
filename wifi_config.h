#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

/* WiFi Connection Settings */
#define WIFI_SSID           "Infun Staff"       // WiFi network name
#define WIFI_PASSWORD       "1n7un@2023Pro"     // WiFi password
#define ESP_STATIC_IP       "192.168.1.50"      // Static IP for ESP32
#define ESP_NETMASK         "255.255.255.0"     // Network mask
#define ESP_GATEWAY         "192.168.1.1"       // Gateway IP (router)
#define MAX_WIFI_RETRIES    10                  // Maximum WiFi connection retries

/* Server Connection Settings */
#define SERVER_PORT         20000               // TCP server port
#define SERVER_ID           "123456"            // Server identification string
#define SCAN_START_IP       254                 // Start IP for scanning (last octet)
#define SCAN_END_IP         250                 // End IP for scanning (last octet)
#define IP_PREFIX           "192.168.1."        // IP address prefix for scanning
#define PING_TIMEOUT_MS     100                 // Ping timeout in milliseconds

/* Gate Control Settings */
#define GPIO_GATE_PIN       4                   // GPIO pin for gate control
#define GATE_OPEN_DURATION  500                 // Gate open duration in milliseconds
#define GATE_DEBOUNCE_TIME  300                 // Debounce time in milliseconds
#define GATE_CMD            "GATE"              // Command to open gate

/* System Settings */
#define SCAN_INTERVAL_MS    5000                // Interval between network scans in milliseconds
#define SCAN_RETRY_DELAY_MS 1000                // Delay between scanning IPs
#define TASK_STACK_SIZE     4096                // Stack size for FreeRTOS tasks

/* Log Settings */
#define LOG_TAG_NETWORK     "NETWORK"           // Log tag for network operations
#define LOG_TAG_SYSTEM      "SYSTEM"            // Log tag for system operations

#endif /* WIFI_CONFIG_H */
