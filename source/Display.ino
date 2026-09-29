void updateOLED()
{
    tcaselect(CH_OLED);


    display.clearDisplay();

    display.setTextColor(
        SSD1306_WHITE);


    // =================================================
    // 頂部
    // =================================================

    display.setTextSize(1);

    display.setCursor(0, 0);


    // 保險狀態

    if (isSafetyError)
        display.print("[ERR]");

    else if (isArmed)
        display.print("[ARM]");

    else
        display.print("[SAF]");


    // Wi-Fi

    display.setCursor(50, 0);


    if (WiFi.status() ==
        WL_CONNECTED)
    {
        display.print("W:OK");
    }
    else
    {
        display.print("W:--");
    }


    // 電池

    display.setCursor(92, 0);

    display.print("B:");

    display.print(batteryPercent);

    display.print("%");


    // 分隔線

    display.drawLine(
        0,
        10,
        128,
        10,
        SSD1306_WHITE);


    // =================================================
    // 中央
    // =================================================

    if (isSafetyError)
    {
        display.setTextSize(2);

        display.setCursor(35, 15);

        display.print("CHECK");


        display.setTextSize(3);

        display.setCursor(25, 32);

        display.print("AMMO");
    }


    else if (inSlowWindow)
    {
        display.setTextSize(2);

        display.setCursor(35, 15);

        display.print("FIRE!");


        display.setCursor(25, 35);

        display.print(
            peakAccInWindow,
            1);

        display.print("G");
    }


    else
    {
        if (ammoCount == -1)
        {
            display.setTextSize(3);

            display.setCursor(38, 22);

            display.print("OUT");
        }
        else
        {
            display.setTextSize(4);

            display.setCursor(52, 18);

            display.print(ammoCount);
        }
    }


    // =================================================
    // 底部
    // =================================================

    display.drawLine(
        0,
        54,
        128,
        54,
        SSD1306_WHITE);


    display.setTextSize(1);


    // 本次開機射擊數

    display.setCursor(0, 56);

    display.print("Now:");

    display.print(normalCount);


    // 永久射擊數

    display.setCursor(70, 56);

    display.print("Tot:");

    display.print(permCount);


    display.display();
}
