#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Wire.h>
#include <WebServer.h>
#include <Adafruit_VL53L0X.h>
#include <Adafruit_VL6180X.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>

#include "Config.h"


// =====================================================
// 全域硬體物件
// =====================================================

Adafruit_SSD1306 display(128, 64, &Wire, -1);
WebServer server(80);

Adafruit_VL53L0X vl53;
Adafruit_VL6180X vl6180;
Adafruit_MPU6050 mpu;

Preferences preferences;


// =====================================================
// 系統狀態
// =====================================================

bool isUpdating = false;
bool isArmed = false;
bool isSafetyError = false;

unsigned long errorDisplayTime = 0;

unsigned int normalCount = 0;
unsigned int permCount = 0;


// =====================================================
// 按鈕
// =====================================================

unsigned long multiBtnPressTime = 0;

bool isMultiBtnPressed = false;
bool actionTriggered = false;


// =====================================================
// 測距 / 箭夾
// =====================================================

int average53 = 0;
int homePosition = 0;

bool hasArrow = false;

int average6180 = 0;
int ammoCount = -1;

bool magHadArrows = false;

int batteryPercent = 0;

int preLoadAmmoCount = -1;


// =====================================================
// VL53 滑動平均
// =====================================================

int readings53[NUM_READINGS_53] = {0};

int readIndex53 = 0;

long total53 = 0;


// =====================================================
// VL6180 滑動平均
// =====================================================

int readings6180[NUM_READINGS_6180] = {0};

int readIndex6180 = 0;

long total6180 = 0;


// =====================================================
// MPU6050
// =====================================================

float baseAccX = 0;
float baseAccY = 0;
float baseAccZ = 0;

float currentG = 0;
float peakAccInWindow = 0;

unsigned long windowStartTime = 0;

bool inSlowWindow = false;


// =====================================================
// 排程計時器
// =====================================================

unsigned long lastVl53Time = 0;
unsigned long lastVl6180Time = 50;

unsigned long lastDisplayTime = 0;

unsigned long lastBatteryTime = 0;

unsigned long lastGithubCheckTime = 0;

unsigned long lastOledHealTime = 0;


// =====================================================
// Setup
// =====================================================

void setup()
{
    Serial.begin(115200);

    Serial.println();
    Serial.println("=================================");
    Serial.println(" Crossbow Firmware");
    Serial.print(" Version: ");
    Serial.println(FIRMWARE_VERSION);
    Serial.println("=================================");


    // -------------------------------------------------
    // I2C
    // -------------------------------------------------

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);


    // -------------------------------------------------
    // GPIO
    // -------------------------------------------------

    pinMode(BTN_SAFETY_PIN, INPUT_PULLUP);
    pinMode(BTN_MULTI_PIN, INPUT_PULLUP);

    analogReadResolution(12);


    // -------------------------------------------------
    // Preferences
    // -------------------------------------------------
    initStorage();

    // -------------------------------------------------
    // OLED
    // -------------------------------------------------

    tcaselect(CH_OLED);

    if (!display.begin(
            SSD1306_SWITCHCAPVCC,
            SCREEN_ADDRESS))
    {
        Serial.println("OLED initialization failed!");

        while (1)
        {
            delay(10);
        }
    }

    display.setTextColor(SSD1306_WHITE);


    // -------------------------------------------------
    // Wi-Fi / OTA
    // -------------------------------------------------

    initWiFi();


    // -------------------------------------------------
    // MPU6050
    // -------------------------------------------------

    initMPU6050();


    // -------------------------------------------------
    // VL53L0X
    // -------------------------------------------------

    initVL53();


    // -------------------------------------------------
    // VL6180X
    // -------------------------------------------------

    initVL6180();


    // -------------------------------------------------
    // VL53 軌道校準
    // -------------------------------------------------

    calibrateRail();


    // -------------------------------------------------
    // 初始化完成
    // -------------------------------------------------

    tcaselect(CH_OLED);

    display.clearDisplay();

    display.setTextSize(2);
    display.setCursor(15, 20);

    display.println("READY");

    display.display();

    delay(1000);

    Serial.println();
    Serial.println("[System] Initialization complete.");
}


// =====================================================
// Loop
// =====================================================

void loop()
{
    if (isUpdating)
    {
        return;
    }


    unsigned long ms = millis();


    // -------------------------------------------------
    // Web OTA
    // -------------------------------------------------

    server.handleClient();


    // -------------------------------------------------
    // 按鈕
    // -------------------------------------------------

    handleButtons(ms);


    // -------------------------------------------------
    // MPU
    // -------------------------------------------------

    handleMPU(ms);


    // -------------------------------------------------
    // VL53 / VL6180 / Battery / OLED
    // -------------------------------------------------

    handlePeriodicTasks(ms);
}
