#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#define ENABLE_BLE_SCANNER 0

static const uint32_t SERIAL_BAUD = 115200;

static const int GPS_RX_PIN = 16;
static const int GPS_TX_PIN = 17;
static const uint32_t GPS_BAUD = 9600;
static const uint8_t MIN_SATELLITES = 4;
static const uint32_t GPS_FIX_STALE_MS = 10000;

static const uint8_t OLED_ADDRESS = 0x3C;
static const int OLED_RESET_PIN = -1;
static const int SCREEN_WIDTH = 128;
static const int SCREEN_HEIGHT = 64;

static const int SD_CS_PIN = 5;
static const uint32_t SUMMARY_WRITE_INTERVAL_MS = 30000;
static const uint32_t LOG_FLUSH_INTERVAL_ROWS = 20;
static const uint32_t LOG_ROTATE_BYTES = 4UL * 1024UL * 1024UL;

static const uint32_t SCAN_INTERVAL_MS = 15000;
static const uint32_t FAST_SCAN_INTERVAL_MS = 5000;
static const uint16_t MAX_UNIQUE_BSSIDS = 450;
static const uint8_t TOP_AP_COUNT = 5;

static const int BUTTON_NEXT_PIN = -1;
static const int BUTTON_SELECT_PIN = -1;
static const uint32_t BUTTON_DEBOUNCE_MS = 75;

#endif
