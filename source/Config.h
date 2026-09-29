#pragma once

// Firmware Version
constexpr const char* FIRMWARE_VERSION = "1.0.0";

// I2C
constexpr uint8_t I2C_SDA_PIN = 8;
constexpr uint8_t I2C_SCL_PIN = 9;

// TCA9548A
constexpr uint8_t TCAADDR = 0x70;

constexpr uint8_t CH_MPU6050 = 0;
constexpr uint8_t CH_OLED = 1;
constexpr uint8_t CH_VL53 = 2;
constexpr uint8_t CH_VL6180 = 3;

// OLED
constexpr uint8_t SCREEN_ADDRESS = 0x3C;

// Buttons

constexpr uint8_t BTN_SAFETY_PIN = 2;
constexpr uint8_t BTN_MULTI_PIN = 3;

// Battery
constexpr uint8_t BATTERY_PIN = 4;

// VL6180X Hardware Offset
constexpr uint8_t HARDWARE_OFFSET = 50;

// VL53 / VL6180 滑動平均
constexpr int NUM_READINGS_53 = 10;
constexpr int NUM_READINGS_6180 = 5;

// 箭夾判定
constexpr int ARROW_THRESHOLD = 8;

// Timing
constexpr unsigned long GITHUB_CHECK_INTERVAL = 300000;
constexpr unsigned long WINDOW_DURATION = 500;
constexpr unsigned long WIFI_RESET_HOLD_TIME = 10000;
constexpr unsigned long OTA_MODE_HOLD_TIME = 3000;
constexpr unsigned long ERROR_MSG_DURATION = 2000;
constexpr unsigned long SENSOR_UPDATE_INTERVAL = 100;
constexpr unsigned long BATTERY_UPDATE_INTERVAL = 1000;
constexpr unsigned long OLED_HEAL_INTERVAL = 10000;

// MPU6050
constexpr float G_THRESHOLD = 5.0;
constexpr float HPF_ALPHA = 0.95;

// GitHub
constexpr const char* URL_VERSION =
  "https://raw.githubusercontent.com/"
  "DuoLily/crossbow-firmware/"
  "refs/heads/main/version.txt";

constexpr const char* URL_FIRMWARE =
  "https://raw.githubusercontent.com/"
  "DuoLily/crossbow-firmware/"
  "refs/heads/main/firmware.bin";