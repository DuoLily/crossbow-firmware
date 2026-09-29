// =====================================================
// OTA Web Page
// =====================================================

const char* update_html PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-TW">

<head>

<meta charset="UTF-8">

<meta name="viewport"
      content="width=device-width,
               initial-scale=1.0">

<title>戰術維護面板</title>

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

<form
    method="POST"
    action="/update"
    enctype="multipart/form-data">

<input
    type="file"
    name="update"
    accept=".bin">

<input
    type="submit"
    value="開始更新">

</form>

</div>

</body>

</html>
)rawliteral";


// =====================================================
// GitHub OTA
// =====================================================

void performGitHubOTA()
{
    WiFiClientSecure client;

    client.setInsecure();


    HTTPClient http;


    Serial.println(
        "\n[GitHub] 檢查更新中...");


    // -----------------------------
    // Version
    // -----------------------------

    http.begin(
        client,
        URL_VERSION);


    if (
        http.GET()
        ==
        HTTP_CODE_OK
    )
    {
        int latestVersion =
            http.getString().toInt();


        Serial.print(
            "[GitHub] Latest version: ");

        Serial.println(
            latestVersion);


        // 注意：
        // 目前 Config 使用 1.0.0
        // 這裡暫時維持舊版整數版本邏輯
        //
        // 下一階段會正式改成
        // Semantic Version
        // 1.0.0 / 1.1.0 / 2.0.0


        if (
            latestVersion >
            1
        )
        {
            Serial.println(
                "[GitHub] 偵測到新版，下載中...");


            isUpdating = true;


            tcaselect(CH_OLED);


            display.clearDisplay();

            display.setTextSize(2);

            display.setCursor(10, 25);

            display.print("UPDATING...");

            display.display();


            http.end();


            // -----------------------------
            // Download Firmware
            // -----------------------------

            http.begin(
                client,
                URL_FIRMWARE);


            if (
                http.GET()
                ==
                HTTP_CODE_OK
            )
            {
                WiFiClient* stream =
                    http.getStreamPtr();


                int contentLength =
                    http.getSize();


                if (
                    Update.begin(
                        contentLength)
                )
                {
                    size_t written =
                        Update.writeStream(
                            *stream);


                    if (
                        written ==
                        contentLength
                    )
                    {
                        Serial.println(
                            "[GitHub] Firmware written successfully");


                        if (
                            Update.end()
                            &&
                            Update.isFinished()
                        )
                        {
                            Serial.println(
                                "[GitHub] OTA SUCCESS");


                            tcaselect(CH_OLED);


                            display.clearDisplay();

                            display.setTextSize(2);

                            display.setCursor(15, 25);

                            display.print("SUCCESS!");

                            display.display();


                            delay(1500);


                            ESP.restart();
                        }
                        else
                        {
                            Serial.println(
                                "[GitHub] OTA 結束失敗");

                            Update.printError(
                                Serial);
                        }
                    }
                    else
                    {
                        Serial.println(
                            "[GitHub] OTA 寫入中斷");

                        Update.printError(
                            Serial);
                    }
                }
                else
                {
                    Serial.println(
                        "[GitHub] OTA 空間不足或初始化失敗");

                    Update.printError(
                        Serial);
                }
            }
            else
            {
                Serial.println(
                    "[GitHub] 無法下載韌體");
            }
        }
    }


    http.end();


    isUpdating = false;
}


// =====================================================
// Wi-Fi Initialization
// =====================================================

void initWiFi()
{
    WiFi.disconnect(
        true,
        true);

    delay(100);


    WiFi.mode(
        WIFI_AP_STA);


    WiFi.setTxPower(
        WIFI_POWER_8_5dBm);


    WiFiManager wm;


    wm.setWiFiAPChannel(6);

    wm.setConnectTimeout(10);

    wm.setConfigPortalTimeout(180);


    // -----------------------------
    // AP Callback
    // -----------------------------

    wm.setAPCallback(
        [](WiFiManager *myWiFiManager)
        {
            tcaselect(CH_OLED);


            display.clearDisplay();

            display.setTextSize(1);

            display.setCursor(0, 0);

            display.println(
                "Crossbow Setup");


            display.drawLine(
                0,
                10,
                128,
                10,
                SSD1306_WHITE);


            display.setCursor(0, 20);

            display.println(
                "Connect WiFi AP:");


            display.setCursor(0, 32);

            display.println(
                myWiFiManager
                    ->getConfigPortalSSID());


            display.setCursor(0, 48);

            display.println(
                "IP: 192.168.4.1");


            display.display();
        }
    );


    // -----------------------------
    // Auto Connect
    // -----------------------------

    if (
        !wm.autoConnect(
            "Crossbow-Setup",
            "12345678")
    )
    {
        // -------------------------
        // Offline Mode
        // -------------------------

        tcaselect(CH_OLED);


        display.clearDisplay();

        display.setTextSize(2);

        display.setCursor(15, 20);

        display.println(
            "OFFLINE");


        display.setTextSize(1);

        display.setCursor(20, 45);

        display.println(
            "Tactical Mode");


        display.display();


        delay(2000);
    }
    else
    {
        // -------------------------
        // Wi-Fi Connected
        // -------------------------

        tcaselect(CH_OLED);


        display.clearDisplay();

        display.setTextSize(1);

        display.setCursor(0, 0);

        display.println(
            "WiFi Connected!");


        display.drawLine(
            0,
            10,
            128,
            10,
            SSD1306_WHITE);


        display.setCursor(0, 20);

        display.println(
            "OTA Web Server:");


        display.setCursor(0, 35);

        display.print(
            "http://");

        display.println(
            WiFi.localIP());


        display.setCursor(0, 52);

        display.println(
            "Ready for update!");


        display.display();


        // -------------------------
        // Web Server
        // -------------------------

        server.on(
            "/",
            []()
            {
                server.send(
                    200,
                    "text/html",
                    update_html);
            }
        );


        server.on(
            "/update",
            HTTP_POST,

            []()
            {
                server.send(
                    200,
                    "text/plain",
                    Update.hasError()
                        ? "Fail"
                        : "OK");

                delay(1000);

                ESP.restart();
            },

            []()
            {
                HTTPUpload& up =
                    server.upload();


                if (
                    up.status ==
                    UPLOAD_FILE_START
                )
                {
                    isUpdating = true;

                    Update.begin(
                        UPDATE_SIZE_UNKNOWN);
                }


                else if (
                    up.status ==
                    UPLOAD_FILE_WRITE
                )
                {
                    Update.write(
                        up.buf,
                        up.currentSize);
                }


                else if (
                    up.status ==
                    UPLOAD_FILE_END
                )
                {
                    Update.end(true);
                }
            }
        );


        server.begin();


        delay(6000);
    }
}
