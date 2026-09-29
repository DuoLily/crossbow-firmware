// =====================================================
// OTA Web Page
// =====================================================

const char* update_html PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-TW">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta http-equiv="refresh" content="2">
<title>十字弓維護面板</title>
<style>
body {
    font-family: sans-serif;
    background: #121212;
    color: #fff;
    text-align: center;
    margin-top: 50px;
}
.card {
    background: #1e1e1e;
    border-radius: 10px;
    padding: 30px;
    margin: auto;
    max-width: 400px;
}
h2 {
  color: #00bcd4;
}
h3 { 
  color: #ff9800; 
  margin-bottom: 5px; 
  }
input[type=file] {
    margin: 20px 0;
    width: 100%;
}
input[type=submit] {
    background: #00bcd4;
    color: #121212;
    border: none;
    padding: 12px;
    border-radius: 5px;
    width: 100%;
    font-weight: bold;
}
</style>
</head>
<body>
<div class="card">
    <h2>⚙️ 手動更新 (OTA)</h2>
    <div style="background: #333; padding: 10px; border-radius: 5px; margin-bottom: 20px;">
        <h3>箭夾測距: %DIST%</h3>
        <h3>判定箭數: %AMMO%</h3>
    </div>
    <form method="POST" action="/update" enctype="multipart/form-data">
        <input type="file" name="update" accept=".bin">
        <input type="submit" value="開始更新">
    </form>
</div>
</body>
</html>
)rawliteral";

// =====================================================
// Version Compare
// 回傳：
// > 0 : newVersion 比 currentVersion 新
// = 0 : 版本相同
// < 0 : newVersion 比 currentVersion 舊
// =====================================================
int compareVersions(const char* currentVersion, const char* newVersion) {
  int currentMajor = 0;
  int currentMinor = 0;
  int currentPatch = 0;

  int newMajor = 0;
  int newMinor = 0;
  int newPatch = 0;

  sscanf(
    currentVersion,
    "%d.%d.%d",
    &currentMajor,
    &currentMinor,
    &currentPatch);

  sscanf(
    newVersion,
    "%d.%d.%d",
    &newMajor,
    &newMinor,
    &newPatch);

  if (newMajor != currentMajor)
    return newMajor - currentMajor;

  if (newMinor != currentMinor)
    return newMinor - currentMinor;

  return newPatch - currentPatch;
}

// =====================================================
// Arduino IDE Wi-Fi OTA
// =====================================================

void initArduinoOTA() {
  ArduinoOTA.setHostname("Crossbow");

  ArduinoOTA.onStart([]() {
    isUpdating = true;

    Serial.println("[ArduinoOTA] Update started");

    tcaselect(CH_OLED);

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(5, 20);
    display.print("UPDATING");
    display.display();
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\n[ArduinoOTA] Update finished");

    tcaselect(CH_OLED);

    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(15, 20);
    display.print("SUCCESS");
    display.display();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    unsigned int percent = (progress * 100) / total;

    Serial.printf(
      "[ArduinoOTA] Progress: %u%%\r",
      percent);

    tcaselect(CH_OLED);

    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(5, 10);
    display.print("UPDATING");

    display.setTextSize(2);
    display.setCursor(40, 35);
    display.print(percent);
    display.print("%");

    display.display();
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf(
      "[ArduinoOTA] Error[%u]\n",
      error);

    tcaselect(CH_OLED);

    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(20, 20);
    display.print("OTA ERR");
    display.display();

    isUpdating = false;
  });

  ArduinoOTA.begin();

  Serial.println("[ArduinoOTA] Wireless upload ready.");
  Serial.print("[ArduinoOTA] Hostname: ");
  Serial.println("Crossbow");
  Serial.print("[ArduinoOTA] IP: ");
  Serial.println(WiFi.localIP());
}


// =====================================================
// GitHub OTA
// =====================================================
void performGitHubOTA() {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  Serial.println();
  Serial.println("[GitHub] Checking for update...");

  // =================================================
  // 1. 下載 GitHub version.txt
  // =================================================
  http.begin(client, URL_VERSION);

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String latestVersion = http.getString();

    latestVersion.trim();

    Serial.print("[GitHub] Current version: ");
    Serial.println(FIRMWARE_VERSION);

    Serial.print("[GitHub] Latest version: ");
    Serial.println(latestVersion);

    // =================================================
    // 2. 比較版本
    // =================================================
    if (compareVersions(
          FIRMWARE_VERSION,
          latestVersion.c_str())
        < 0) {
      Serial.println("[GitHub] New firmware detected.");

      isUpdating = true;

      // =================================================
      // OLED：更新中
      // =================================================
      tcaselect(CH_OLED);

      display.clearDisplay();

      display.setTextSize(2);
      display.setCursor(10, 10);
      display.print("UPDATING");

      display.setTextSize(1);
      display.setCursor(15, 40);
      display.print(latestVersion);

      display.display();

      http.end();

      // =================================================
      // 3. 下載 Firmware
      // =================================================
      http.begin(client, URL_FIRMWARE);

      int firmwareCode = http.GET();

      if (firmwareCode == HTTP_CODE_OK) {
        WiFiClient* stream = http.getStreamPtr();

        int contentLength = http.getSize();

        Serial.print("[GitHub] Firmware size: ");
        Serial.println(contentLength);

        if (contentLength > 0 && Update.begin(contentLength)) {
          size_t written =
            Update.writeStream(*stream);

          Serial.print("[GitHub] Written: ");
          Serial.println(written);

          if (written == contentLength) {
            Serial.println(
              "[GitHub] Firmware written successfully.");

            if (Update.end() && Update.isFinished()) {
              Serial.println(
                "[GitHub] OTA SUCCESS");

              tcaselect(CH_OLED);

              display.clearDisplay();

              display.setTextSize(2);
              display.setCursor(15, 10);
              display.print("SUCCESS!");

              display.setTextSize(1);
              display.setCursor(25, 40);
              display.print(latestVersion);

              display.display();

              delay(1500);

              ESP.restart();
            } else {
              Serial.println(
                "[GitHub] OTA finish failed.");

              Update.printError(Serial);
            }
          } else {
            Serial.println(
              "[GitHub] Firmware write interrupted.");

            Update.printError(Serial);
          }
        } else {
          Serial.println(
            "[GitHub] OTA space/init failed.");

          Update.printError(Serial);
        }
      } else {
        Serial.print(
          "[GitHub] Firmware download failed. HTTP: ");

        Serial.println(firmwareCode);
      }
    } else {
      Serial.println(
        "[GitHub] Firmware is already up to date.");
    }
  } else {
    Serial.print(
      "[GitHub] Version check failed. HTTP: ");

    Serial.println(httpCode);
  }

  http.end();

  isUpdating = false;
}

// =====================================================
// Wi-Fi Initialization
// =====================================================

void initWiFi() {
  // -------------------------------------------------
  // Reset Wi-Fi
  // -------------------------------------------------

  WiFi.disconnect(true, true);

  delay(100);

  WiFi.mode(WIFI_AP_STA);

  WiFi.setTxPower(WIFI_POWER_8_5dBm);

  // -------------------------------------------------
  // WiFiManager
  // -------------------------------------------------

  WiFiManager wm;

  wm.setWiFiAPChannel(6);

  wm.setConnectTimeout(10);

  wm.setConfigPortalTimeout(180);


  // -------------------------------------------------
  // AP Callback
  // -------------------------------------------------

  wm.setAPCallback([](WiFiManager* myWiFiManager) {
    tcaselect(CH_OLED);

    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);

    display.setCursor(0, 0);
    display.println("Crossbow Setup");

    display.drawLine(
      0,
      10,
      128,
      10,
      SSD1306_WHITE);

    display.setCursor(0, 20);
    display.println("Connect WiFi AP:");

    display.setCursor(0, 32);
    display.println(
      myWiFiManager->getConfigPortalSSID());

    display.setCursor(0, 48);
    display.println("IP: 192.168.4.1");

    display.display();
  });


  // -------------------------------------------------
  // Auto Connect
  // -------------------------------------------------

  if (!wm.autoConnect(
        "Crossbow-Setup",
        "12345678")) {
    // -------------------------------------------------
    // Offline Mode
    // -------------------------------------------------

    tcaselect(CH_OLED);

    display.clearDisplay();

    display.setTextSize(2);

    display.setCursor(15, 20);

    display.println("OFFLINE");

    display.setTextSize(1);

    display.setCursor(20, 45);

    display.println("Tactical Mode");

    display.display();

    delay(2000);
  } else {
    // -------------------------------------------------
    // Wi-Fi Connected
    // -------------------------------------------------

    Serial.println();
    Serial.println("==============================");
    Serial.println("Wi-Fi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("==============================");


    // -------------------------------------------------
    // OLED
    // -------------------------------------------------

    tcaselect(CH_OLED);

    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);

    display.setCursor(0, 0);

    display.println("WiFi Connected!");

    display.drawLine(
      0,
      10,
      128,
      10,
      SSD1306_WHITE);

    display.setCursor(0, 20);

    display.println("OTA Web Server:");

    display.setCursor(0, 35);

    display.print("http://");

    display.println(WiFi.localIP());

    display.setCursor(0, 52);

    display.println("Ready for update!");

    display.display();


    // -------------------------------------------------
    // Web OTA Server
    // -------------------------------------------------

    server.on(
      "/",
      []() {
        String dynamicHtml = String(update_html);  // 將 PROGMEM 中的靜態網頁載入為動態字串

        dynamicHtml.replace("%DIST%", String(average6180));  // 替換字串內的佔位符為實際的感測器變數
        dynamicHtml.replace("%AMMO%", String(ammoCount));
        server.send(
          200,
          "text/html",
          update_html);
      });


    server.on(
      "/update",
      HTTP_POST,

      []() {
        server.send(
          200,
          "text/plain",
          Update.hasError()
            ? "Fail"
            : "OK");

        delay(1000);

        ESP.restart();
      },

      []() {
        HTTPUpload& up = server.upload();

        if (
          up.status == UPLOAD_FILE_START) {
          isUpdating = true;

          Serial.println(
            "[WebOTA] Update started.");

          if (
            !Update.begin(
              UPDATE_SIZE_UNKNOWN)) {
            Update.printError(
              Serial);
          }
        } else if (
          up.status == UPLOAD_FILE_WRITE) {
          if (
            Update.write(
              up.buf,
              up.currentSize)
            != up.currentSize) {
            Update.printError(
              Serial);
          }
        } else if (
          up.status == UPLOAD_FILE_END) {
          if (Update.end(true)) {
            Serial.println(
              "[WebOTA] Update finished.");
          } else {
            Update.printError(
              Serial);
          }
        }
      });


    server.begin();

    Serial.println(
      "[WebOTA] Web server started.");


    // -------------------------------------------------
    // Arduino IDE OTA
    // -------------------------------------------------

    initArduinoOTA();


    // -------------------------------------------------
    // Show IP for 6 seconds
    // -------------------------------------------------

    delay(6000);
  }
}
