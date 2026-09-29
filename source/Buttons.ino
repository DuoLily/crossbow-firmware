void handleButtons(unsigned long ms) {
  // =================================================
  // Safety Button
  // =================================================
  if (digitalRead(BTN_SAFETY_PIN) == LOW && !isArmed && !isSafetyError) {
    bool isMagDropped = (ammoCount == (preLoadAmmoCount - 1));
    bool isLastArrowMisread =
      (preLoadAmmoCount == 1 && ammoCount == 1 && hasArrow);

    if (ammoCount != -1) {
      // 情況 1：完美上膛
      if (isMagDropped && hasArrow) {
        isArmed = true;
        isSafetyError = false;
      }
      // 情況 2：最後一支箭容錯
      else if (isLastArrowMisread) {
        ammoCount = 0;
        isArmed = true;
        isSafetyError = false;

        Serial.println("[Safety] Last arrow tolerance activated.");
      }
      // 情況 3：箭夾下落，但軌道沒有箭
      else if (isMagDropped && !hasArrow) {
        isSafetyError = true;
        errorDisplayTime = ms;

        Serial.println("[Safety] ERROR: Magazine dropped but no arrow detected.");
      }
      // 情況 4：箭夾未下落，但軌道有箭
      else if (!isMagDropped && hasArrow) {
        isSafetyError = true;
        errorDisplayTime = ms;

        Serial.println("[Safety] ERROR: Arrow detected but magazine did not drop.");
      }
      // 情況 5：其他未知錯誤
      else {
        isSafetyError = true;
        errorDisplayTime = ms;

        Serial.println("[Safety] ERROR: Unknown loading state.");
      }
    } else {
      // 沒有箭夾，禁止上膛
      isSafetyError = true;
      errorDisplayTime = ms;

      Serial.println("[Safety] ERROR: No magazine.");
    }
  }
  // =================================================
  // Safety Error Timeout
  // =================================================
  else if (
    isSafetyError && (ms - errorDisplayTime > ERROR_MSG_DURATION)) {
    isSafetyError = false;
  }


  // =================================================
  // Multi Button
  // =================================================
  if (digitalRead(BTN_MULTI_PIN) == LOW) {
    // -------------------------------------------------
    // 按下瞬間
    // -------------------------------------------------
    if (!isMultiBtnPressed) {
      isMultiBtnPressed = true;
      multiBtnPressTime = ms;
      actionTriggered = false;
    } else {
      unsigned long hold = ms - multiBtnPressTime;

      // -------------------------------------------------
      // 10 秒：Wi-Fi 設定重置
      // -------------------------------------------------
      if (
        hold >= WIFI_RESET_HOLD_TIME && !actionTriggered) {
        actionTriggered = true;

        WiFiManager wm;
        wm.resetSettings();

        Serial.println("[WiFi] Settings reset.");

        ESP.restart();
      }

      // -------------------------------------------------
      // 3 秒：OTA Mode 提示
      // -------------------------------------------------
      else if (
        hold >= OTA_MODE_HOLD_TIME && hold < WIFI_RESET_HOLD_TIME && !actionTriggered) {
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
  // Multi Button Released
  // =================================================
  else if (isMultiBtnPressed) {
    isMultiBtnPressed = false;

    unsigned long hold =
      ms - multiBtnPressTime;

    if (!actionTriggered) {
      // -------------------------------------------------
      // 3 秒以上：OTA Mode
      // -------------------------------------------------
      if (hold >= OTA_MODE_HOLD_TIME) {
        isUpdating = true;

        Serial.println("[OTA] OTA mode requested.");
      }
      // -------------------------------------------------
      // 短按：本次射擊數歸零
      // -------------------------------------------------
      else if (hold > 50) {
        normalCount = 0;

        Serial.println("[Counter] Normal count reset.");
      }
    }
  }
}