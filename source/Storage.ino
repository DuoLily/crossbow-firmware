// Storage Initialization

void initStorage()
{
    if (!preferences.begin("counterSpace", false))
    {
        Serial.println("[Storage] Preferences initialization failed!");
        return;
    }

    permCount = preferences.getUInt("permVal", 0);

    Serial.print("[Storage] Permanent count: ");
    Serial.println(permCount);
}

// Save Permanent Count

void savePermanentCount(unsigned int value)
{
    preferences.putUInt("permVal", value);
}
