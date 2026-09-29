// =====================================================
// TCA9548A Channel Select
// =====================================================
void tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}

// =====================================================
// MPU6050 Initialization
// =====================================================
void initMPU6050() {
  tcaselect(CH_OLED);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("Init MPU6050...");
  display.display();

  tcaselect(CH_MPU6050);
  if (!mpu.begin()) {
    Serial.println("[ERROR] MPU6050 initialization failed!");
    while (1) { delay(10); }
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setFilterBandwidth(MPU6050_BAND_184_HZ);
  Serial.println("[MPU6050] OK");
}

// =====================================================
// VL53L0X Initialization
// =====================================================
void initVL53() {
  tcaselect(CH_OLED);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("Init VL53L0X...");
  display.display();

  tcaselect(CH_VL53);
  if (!vl53.begin()) {
    Serial.println("[ERROR] VL53L0X initialization failed!");
    while (1) { delay(10); }
  }

  vl53.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_DEFAULT);
  Serial.println("[VL53L0X] OK");
}

// =====================================================
// VL6180X Initialization
// =====================================================
void initVL6180() {
  tcaselect(CH_OLED);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("Init VL6180X...");
  display.display();

  tcaselect(CH_VL6180);
  if (!vl6180.begin()) {
    Serial.println("[ERROR] VL6180X initialization failed!");
    while (1) { delay(10); }
  }

  // Hardware offset
  Wire.beginTransmission(0x29);
  Wire.write(0x00);
  Wire.write(0x24);
  Wire.write(HARDWARE_OFFSET);
  Wire.endTransmission();

  Serial.println("[VL6180X] OK");
}

// =====================================================
// VL53 Rail Calibration
// =====================================================
void calibrateRail() {
  tcaselect(CH_OLED);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(5, 25);
  display.println("Calibrating Rail...");
  display.display();

  Serial.println("\n[VL53] Starting rail calibration...");
  long calibSum = 0;
  int vCnt = 0;

  tcaselect(CH_VL53);

  for (int i = 0; i < 20; i++) {
    Serial.print("Calibration reading ");
    Serial.print(i + 1);
    Serial.print(" ... ");

    VL53L0X_RangingMeasurementData_t m;
    vl53.rangingTest(&m, false);
    Serial.println("OK");

    if (m.RangeStatus != 4) {
      calibSum += m.RangeMilliMeter;
      vCnt++;
    }
    delay(20);
  }

  homePosition = (vCnt > 0) ? (calibSum / vCnt) : 200;

  // 預填滑動平均
  for (int i = 0; i < NUM_READINGS_53; i++) {
    readings53[i] = homePosition;
    total53 += homePosition;
  }

  Serial.print("[VL53] Home Position = ");
  Serial.print(homePosition);
  Serial.println(" mm");
}

// =====================================================
// MPU6050 Update (Shot Detection)
// =====================================================
void handleMPU(unsigned long ms) {
  tcaselect(CH_MPU6050);

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Low-pass filter for baseline gravity tracking
  baseAccX = HPF_ALPHA * baseAccX + (1.0 - HPF_ALPHA) * a.acceleration.x;
  baseAccY = HPF_ALPHA * baseAccY + (1.0 - HPF_ALPHA) * a.acceleration.y;
  baseAccZ = HPF_ALPHA * baseAccZ + (1.0 - HPF_ALPHA) * a.acceleration.z;

  // Calculate dynamic acceleration magnitude
  float totalDynAcc = sqrt(pow(a.acceleration.x - baseAccX, 2) + pow(a.acceleration.y - baseAccY, 2) + pow(a.acceleration.z - baseAccZ, 2));

  currentG = totalDynAcc / 9.81;

  // 射擊判定 (Shot Detection)
  if (isArmed && !inSlowWindow && currentG > G_THRESHOLD) {
    inSlowWindow = true;
    windowStartTime = ms;
    peakAccInWindow = currentG;

    normalCount++;
    permCount++;
    savePermanentCount(permCount);

    isArmed = false;
  }

  // 射擊視窗 (Tracking peak G-force during recoil)
  if (inSlowWindow) {
    if (currentG > peakAccInWindow) {
      peakAccInWindow = currentG;
    }

    if (ms - windowStartTime >= WINDOW_DURATION) {
      inSlowWindow = false;
    }
  }
}

// =====================================================
// VL53 Update (Rail Distance)
// =====================================================
void updateVL53(unsigned long ms) {
  if (ms - lastVl53Time < SENSOR_UPDATE_INTERVAL) return;
  lastVl53Time = ms;

  tcaselect(CH_VL53);
  VL53L0X_RangingMeasurementData_t measure;
  vl53.rangingTest(&measure, false);

  if (measure.RangeStatus != 4) {
    total53 = total53 + measure.RangeMilliMeter - readings53[readIndex53];
    readings53[readIndex53] = measure.RangeMilliMeter;
    readIndex53 = (readIndex53 + 1) % NUM_READINGS_53;
    average53 = total53 / NUM_READINGS_53;

    hasArrow = (average53 <= (homePosition - ARROW_THRESHOLD));
  }
}

// =====================================================
// Raw Arrow Count Mapping
// =====================================================
int getRawArrowCount(int dist) {
  // 無箭夾 (表格測量值 72~75) -> 界線抓 68
  if (dist >= 68) return -1;
  // 空箭夾 (表格測量值 60~62) -> 因與 1 支箭重疊嚴重，界線極限抓 62
  else if (dist >= 62) return 0;
  // 1支箭 (表格測量值 58~61) -> 界線抓 56
  else if (dist >= 56) return 1;
  // 2支箭 (表格測量值 52~54) -> 界線抓 49
  else if (dist >= 49) return 2;
  // 3支箭 (表格測量值 44~47) -> 界線抓 41
  else if (dist >= 41) return 3;
  // 4支箭 (表格測量值 36~38) -> 界線抓 34
  else if (dist >= 34) return 4;
  // 5支箭 (表格測量值 30~32) -> 界線抓 31
  else if (dist > 30) return 5;
  // 6支箭 (表格測量值 28~30 及其以下) -> 小於等於 30 一律視為 6 支
  else return 6;
}

// =====================================================
// 溫度補償與動態基準變數
// =====================================================
int driftOffset = 0;
const int DEFAULT_EMPTY_DIST = 73;

// 計算 VL6180 歷史讀數的標準差 (新增在這裡)
float getStdDev6180() {
  float mean = average6180;
  float varianceSum = 0;
  for (int i = 0; i < NUM_READINGS_6180; i++) {
    varianceSum += sq(readings6180[i] - mean);
  }
  return sqrt(varianceSum / NUM_READINGS_6180);
}

// =====================================================
// VL6180 Update (Magazine Distance)
// =====================================================
void updateVL6180(unsigned long ms) {
  // 頻率控制：依照 SENSOR_UPDATE_INTERVAL (100ms) 執行[cite: 3]
  if (ms - lastVl6180Time < SENSOR_UPDATE_INTERVAL) return;
  lastVl6180Time = ms;

  tcaselect(CH_VL6180);  // 切換 TCA9548A 通道[cite: 2, 3]
  uint8_t rawRange = vl6180.readRange();

  if (vl6180.readRangeStatus() == VL6180X_ERROR_NONE) {
    //1. 滑動平均更新
    total6180 = total6180 + rawRange - readings6180[readIndex6180];
    readings6180[readIndex6180] = rawRange;
    readIndex6180 = (readIndex6180 + 1) % NUM_READINGS_6180;
    average6180 = total6180 / NUM_READINGS_6180;

    // 2. 計算標準差 (波動程度)
    float stdDev = getStdDev6180();

    // 3. 慢速高精度溫度補償 (Auto-Tare)
    static unsigned long stableStartTime = 0;
    const unsigned long CALIBRATION_DELAY = 3000;  // 需連續穩定 3 秒

    // 判斷當下是否處於無箭夾的穩態 (> 68 且波動小於 1.0)
    if (average6180 > 68 && stdDev < 1.0) {
      // 如果穩定時間達標，更新飄移值
      if (ms - stableStartTime >= CALIBRATION_DELAY) {
        driftOffset = average6180 - DEFAULT_EMPTY_DIST;
      }
    } else {
      stableStartTime = ms;  // 只要數值一波動或裝上箭夾，計時器立刻歸零重置
    }

    // 4. 安裝暫態過濾 (Transient Noise Rejection)
    if (stdDev > 2.0) {  // 如果距離跳動過大 (標準差 > 2.0)，代表正在裝卸箭夾或劇烈晃動，不更新箭數
      return;
    }

    // 5. 套用補償並轉換為箭矢數量
    int compensatedDist = average6180 - driftOffset;
    int newAmmoCount = getRawArrowCount(compensatedDist);  // 呼叫更新後的查表函式

    // 6. 單向遞減狀態機 (接管原本的 if-else 邏輯)
    if (newAmmoCount == -1) {
      ammoCount = -1;  // 無箭夾狀態
      magHadArrows = false;
    } else if (ammoCount == -1) {
      ammoCount = newAmmoCount;  // 狀態：剛剛裝上新箭夾，鎖定初始數量
      if (ammoCount > 0) magHadArrows = true;
    } else {
      if (newAmmoCount < ammoCount) {  // 狀態：射擊中 (箭數只能減少或持平，不能增加)
        ammoCount = newAmmoCount;
      }
    }
  }
}
// =====================================================
// Battery Update
// =====================================================
void updateBattery(unsigned long ms) {
  if (ms - lastBatteryTime < BATTERY_UPDATE_INTERVAL) return;
  lastBatteryTime = ms;

  // Voltage calculation with hardware multiplier (approx 2.0x for voltage divider)
  float voltage = analogReadMilliVolts(BATTERY_PIN) / 1000.0 * 2.008;

  batteryPercent = constrain(map(voltage * 100, 320, 420, 0, 100), 0, 100);
}

// =====================================================
// Periodic Tasks Main Handler
// =====================================================
void handlePeriodicTasks(unsigned long ms) {
  updateBattery(ms);
  updateVL53(ms);
  updateVL6180(ms);

  // -----------------------------
  // 彈匣離開軌道時記錄目前箭數
  // -----------------------------
  if (!hasArrow) {
    preLoadAmmoCount = ammoCount;
  }

  // -----------------------------
  // OLED & Serial Output
  // -----------------------------
  if (ms - lastDisplayTime >= SENSOR_UPDATE_INTERVAL) {
    lastDisplayTime = ms;
    updateOLED();

    Serial.print("[滑軌-VL53L0X] 距離: ");
    Serial.print(average53);
    Serial.print(" mm (是否有箭: ");
    Serial.print(hasArrow ? "是" : "否");
    Serial.print(")  ||  [箭夾-VL6180X] 距離: ");
    Serial.print(average6180);
    Serial.print(" mm (計算備彈: ");

    if (ammoCount == -1) {
      Serial.println("空匣)");
    } else {
      Serial.print(ammoCount);
      Serial.println(" 發)");
    }
  }

  // -----------------------------
  // OLED Heal (Periodic Display Re-init)
  // -----------------------------
  if (ms - lastOledHealTime >= OLED_HEAL_INTERVAL) {
    lastOledHealTime = ms;
    tcaselect(CH_OLED);
    display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS, false, false);
  }
}