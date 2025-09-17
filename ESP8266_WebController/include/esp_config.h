#ifndef ESP_CONFIG_H
#define ESP_CONFIG_H

// WiFi Configuration
#define WIFI_SSID "RobotController"        // AP mode SSID
#define WIFI_PASSWORD "robot123"           // AP mode password
#define AP_IP_ADDRESS IPAddress(192,168,4,1)
#define AP_GATEWAY IPAddress(192,168,4,1)
#define AP_SUBNET IPAddress(255,255,255,0)

// Web server port
#define WEB_SERVER_PORT 80

// Serial communication with Arduino Mega
#define MEGA_SERIAL_BAUD 115200

// Command timeouts and intervals
#define COMMAND_TIMEOUT_MS 5000
#define HEARTBEAT_INTERVAL_MS 1000
#define STATUS_UPDATE_INTERVAL_MS 500

// Maximum command length
#define MAX_COMMAND_LENGTH 200

// Robot control constants
#define DEFAULT_SPEED 150
#define MAX_SPEED 255
#define MIN_SPEED -255

#endif // ESP_CONFIG_H
