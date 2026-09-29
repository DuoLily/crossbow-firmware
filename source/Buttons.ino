void handleButtons(unsigned long ms)
{
    // =================================================
    // Safety Button
    // =================================================

    if (
        digitalRead(BTN_SAFETY_PIN) == LOW
        &&
        !isArmed
        &&
        !isSafetyError
    )
    {
        if (
            hasArrow
            &&
            ammoCount != -1
            &&
            ammoCount ==
                (preLoadAmmoCount - 1)
        )
        {
            isArmed = true;

            isSafetyError = false;
        }
        else
        {
            isSafetyError = true;

            errorDisplayTime = ms;
        }
    }


    // =================================================
    // Safety Error timeout
    // =================================================

    else if (
        isSafetyError
        &&
        (
            ms - errorDisplayTime
            >
            ERROR_MSG_DURATION
        )
    )
    {
        isSafetyError = false;
    }


    // =================================================
    // Multi Button
    // =================================================

    if (
        digitalRead(BTN_MULTI_PIN)
        ==
        LOW
    )
    {
        // -----------------------------
        // 按下瞬間
        // -----------------------------

        if (!isMultiBtnPressed)
        {
            isMultiBtnPressed = true;

            multiBtnPressTime = ms;

            actionTriggered = false;
        }


        // -----------------------------
        // 持續按住
        // -----------------------------

        else
        {
            unsigned long hold =
                ms - multiBtnPressTime;


            // 10 秒：
            // Wi-Fi 設定重置

            if (
                hold >=
                WIFI_RESET_HOLD_TIME
                &&
                !actionTriggered
            )
            {
                actionTriggered = true;


                WiFiManager wm;

                wm.resetSettings();


                Serial.println(
                    "[WiFi] Settings reset");


                ESP.restart();
            }


            // 3 秒：
            // OTA Mode 顯示

            else if (
                hold >=
                OTA_MODE_HOLD_TIME
                &&
                hold <
                WIFI_RESET_HOLD_TIME
                &&
                !actionTriggered
            )
            {
                tcaselect(CH_OLED);


                display.clearDisplay();

                display.setTextSize(2);

                display.setCursor(5, 10);

                display.print("OTA MODE");

                display.display();
            }
        }
    }


    // =================================================
    // 按鈕放開
    // =================================================

    else if (isMultiBtnPressed)
    {
        isMultiBtnPressed = false;


        unsigned long hold =
            ms - multiBtnPressTime;


        if (!actionTriggered)
        {
            // -----------------------------
            // 3 秒以上
            // OTA Mode
            // -----------------------------

            if (
                hold >=
                OTA_MODE_HOLD_TIME
            )
            {
                isUpdating = true;
            }


            // -----------------------------
            // 一般短按
            // 射擊數歸零
            // -----------------------------

            else if (hold > 50)
            {
                normalCount = 0;
            }
        }
    }
}
