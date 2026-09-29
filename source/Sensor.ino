// =====================================================
// TCA9548A Channel Select
// =====================================================

void tcaselect(uint8_t i)
{
    if (i > 7)
        return;

    Wire.beginTransmission(TCAADDR);

    Wire.write(1 << i);

    Wire.endTransmission();
}


// =====================================================
// MPU6050 Initialization
// =====================================================

void initMPU6050()
{
    tcaselect(CH_OLED);

    display.clearDisplay();

    display.setTextSize(1);

    display.setCursor(0, 20);

    display.println("Init MPU6050...");

    display.display();


    tcaselect(CH_MPU6050);

    if (!mpu.begin())
    {
        Serial.println("[ERROR] MPU6050 initialization failed!");

        while (1)
        {
            delay(10);
        }
    }


    mpu.setAccelerometerRange(
        MPU6050_RANGE_16_G);

    mpu.setFilterBandwidth(
        MPU6050_BAND_184_HZ);


    Serial.println("[MPU6050] OK");
}


// =====================================================
// VL53L0X Initialization
// =====================================================

void initVL53()
{
    tcaselect(CH_OLED);

    display.clearDisplay();

    display.setTextSize(1);

    display.setCursor(0, 20);

    display.println("Init VL53L0X...");

    display.display();


    tcaselect(CH_VL53);

    if (!vl53.begin())
    {
        Serial.println("[ERROR] VL53L0X initialization failed!");

        while (1)
        {
            delay(10);
        }
    }


    vl53.configSensor(
        Adafruit_VL53L0X::VL53L0X_SENSE_DEFAULT);


    Serial.println("[VL53L0X] OK");
}


// =====================================================
// VL6180X Initialization
// =====================================================

void initVL6180()
{
    tcaselect(CH_OLED);

    display.clearDisplay();

    display.setTextSize(1);

    display.setCursor(0, 20);

    display.println("Init VL6180X...");

    display.display();


    tcaselect(CH_VL6180);

    if (!vl6180.begin())
    {
        Serial.println("[ERROR] VL6180X initialization failed!");

        while (1)
        {
            delay(10);
        }
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

void calibrateRail()
{
    tcaselect(CH_OLED);

    display.clearDisplay();

    display.setTextSize(1);

    display.setCursor(5, 25);

    display.println("Calibrating Rail...");

    display.display();


    Serial.println();
    Serial.println(
        "[VL53] Starting rail calibration...");


    long calibSum = 0;

    int vCnt = 0;


    tcaselect(CH_VL53);


    for (int i = 0; i < 20; i++)
    {
        Serial.print(
            "Calibration reading ");

        Serial.print(i + 1);

        Serial.print(" ... ");


        VL53L0X_RangingMeasurementData_t m;


        vl53.rangingTest(&m, false);


        Serial.println("OK");


        if (m.RangeStatus != 4)
        {
            calibSum += m.RangeMilliMeter;

            vCnt++;
        }


        delay(20);
    }


    homePosition =
        vCnt > 0
            ? calibSum / vCnt
            : 200;


    // 預填滑動平均

    for (int i = 0;
         i < NUM_READINGS_53;
         i++)
    {
        readings53[i] = homePosition;

        total53 += homePosition;
    }


    Serial.print(
        "[VL53] Home Position = ");

    Serial.print(homePosition);

    Serial.println(" mm");
}


// =====================================================
// MPU6050 Update
// =====================================================

void handleMPU(unsigned long ms)
{
    tcaselect(CH_MPU6050);


    sensors_event_t a;
    sensors_event_t g;
    sensors_event_t temp;


    mpu.getEvent(
        &a,
        &g,
        &temp);


    // Low-pass baseline

    baseAccX =
        HPF_ALPHA * baseAccX
        +
        (1.0 - HPF_ALPHA)
        * a.acceleration.x;


    baseAccY =
        HPF_ALPHA * baseAccY
        +
        (1.0 - HPF_ALPHA)
        * a.acceleration.y;


    baseAccZ =
        HPF_ALPHA * baseAccZ
        +
        (1.0 - HPF_ALPHA)
        * a.acceleration.z;


    float totalDynAcc =
        sqrt(
            pow(
                a.acceleration.x - baseAccX,
                2)
            +
            pow(
                a.acceleration.y - baseAccY,
                2)
            +
            pow(
                a.acceleration.z - baseAccZ,
                2)
        );


    currentG =
        totalDynAcc / 9.81;


    // 射擊判定

    if (isArmed &&
        !inSlowWindow &&
        currentG > G_THRESHOLD)
    {
        inSlowWindow = true;

        windowStartTime = ms;

        peakAccInWindow =
            currentG;

        normalCount++;
        permCount++;
        savePermanentCount(permCount);


        isArmed = false;
    }


    // 射擊視窗

    if (inSlowWindow)
    {
        if (currentG >
            peakAccInWindow)
        {
            peakAccInWindow =
                currentG;
        }


        if (ms -
            windowStartTime >=
            WINDOW_DURATION)
        {
            inSlowWindow = false;
        }
    }
}


// =====================================================
// VL53 Update
// =====================================================

void updateVL53(unsigned long ms)
{
    if (ms - lastVl53Time <
        SENSOR_UPDATE_INTERVAL)
    {
        return;
    }


    lastVl53Time = ms;


    tcaselect(CH_VL53);


    VL53L0X_RangingMeasurementData_t measure;


    vl53.rangingTest(
        &measure,
        false);


    if (measure.RangeStatus != 4)
    {
        total53 +=
            measure.RangeMilliMeter
            -
            readings53[readIndex53];


        readings53[readIndex53] =
            measure.RangeMilliMeter;


        readIndex53 =
            (readIndex53 + 1)
            %
            NUM_READINGS_53;


        average53 =
            total53 /
            NUM_READINGS_53;


        hasArrow =
            (
                average53 <=
                (
                    homePosition
                    -
                    ARROW_THRESHOLD
                )
            );
    }
}


// =====================================================
// Raw Arrow Count Mapping
// =====================================================

int getRawArrowCount(int dist)
{
    if (dist >= 49)
        return -1;

    else if (dist > 46)
        return 0;

    else if (dist > 39)
        return 1;

    else if (dist > 34)
        return 2;

    else if (dist > 28)
        return 3;

    else if (dist > 23)
        return 4;

    else if (dist > 19)
        return 5;

    else if (dist > 14)
        return 6;

    else
        return 7;
}


// =====================================================
// VL6180 Update
// =====================================================

void updateVL6180(unsigned long ms)
{
    if (ms - lastVl6180Time <
        SENSOR_UPDATE_INTERVAL)
    {
        return;
    }


    lastVl6180Time = ms;


    tcaselect(CH_VL6180);


    uint8_t rawRange =
        vl6180.readRange();


    if (
        vl6180.readRangeStatus()
        ==
        VL6180X_ERROR_NONE
    )
    {
        total6180 +=
            rawRange
            -
            readings6180[
                readIndex6180];


        readings6180[
            readIndex6180]
            =
            rawRange;


        readIndex6180 =
            (readIndex6180 + 1)
            %
            NUM_READINGS_6180;


        average6180 =
            total6180 /
            NUM_READINGS_6180;


        int rawAmmo =
            getRawArrowCount(
                average6180);


        if (rawAmmo > 0)
        {
            magHadArrows = true;

            ammoCount = rawAmmo;
        }
        else if (rawAmmo == 0)
        {
            ammoCount =
                magHadArrows
                    ? 0
                    : -1;
        }
        else
        {
            magHadArrows = false;

            ammoCount = -1;
        }
    }
}


// =====================================================
// Battery
// =====================================================

void updateBattery(unsigned long ms)
{
    if (ms - lastBatteryTime <
        BATTERY_UPDATE_INTERVAL)
    {
        return;
    }


    lastBatteryTime = ms;


    float voltage =
        analogReadMilliVolts(
            BATTERY_PIN)
        /
        1000.0
        *
        2.008;


    batteryPercent =
        constrain(
            map(
                voltage * 100,
                320,
                420,
                0,
                100
            ),
            0,
            100
        );
}


// =====================================================
// Periodic Tasks
// =====================================================

void handlePeriodicTasks(unsigned long ms)
{
    updateBattery(ms);

    updateVL53(ms);

    updateVL6180(ms);


    // -----------------------------
    // 彈匣離開軌道時記錄目前箭數
    // -----------------------------

    if (!hasArrow)
    {
        preLoadAmmoCount =
            ammoCount;
    }


    // -----------------------------
    // OLED
    // -----------------------------

    if (ms - lastDisplayTime >=
        SENSOR_UPDATE_INTERVAL)
    {
        lastDisplayTime = ms;

        updateOLED();


        Serial.print(
            "[滑軌-VL53L0X] 距離: ");

        Serial.print(average53);

        Serial.print(
            " mm (是否有箭: ");

        Serial.print(
            hasArrow
                ? "是"
                : "否");


        Serial.print(
            ")  ||  [箭夾-VL6180X] 距離: ");

        Serial.print(average6180);

        Serial.print(
            " mm (計算備彈: ");


        if (ammoCount == -1)
        {
            Serial.println("空匣)");
        }
        else
        {
            Serial.print(ammoCount);

            Serial.println(" 發)");
        }
    }


    // -----------------------------
    // OLED Heal
    // -----------------------------

    if (ms - lastOledHealTime >=
        OLED_HEAL_INTERVAL)
    {
        lastOledHealTime = ms;


        tcaselect(CH_OLED);


        display.begin(
            SSD1306_SWITCHCAPVCC,
            SCREEN_ADDRESS,
            false,
            false);
    }
}
