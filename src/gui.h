void handleOTAUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final)
{
    if (!index)
    {
#if defined(DEBUG) || defined(GUI_DEBUG)
        Serial1.print(F("UploadStart: "));
        Serial1.println(filename.c_str());
#endif
         // calculate sketch space required for the update, for ESP32 use the max constant
#if defined(ESP32)
        if (!Update.begin(UPDATE_SIZE_UNKNOWN))
#else
        const uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace))
#endif
        {
            // start with max available size
            Update.printError(Serial);
        }
#if defined(ESP8266)
        Update.runAsync(true);
#endif
    }

    if (len)
    {
        Update.write(data, len);
    }

    // if the final flag is set then this is the last frame of data
    if (final)
    {
        if (Update.end(true))
        {
          
#if defined(DEBUG) || defined(GUI_DEBUG)
            // true to set the size to the current progress
            Serial1.print(F("Update Success: "));
            Serial1.print(index + len);
            Serial1.println(F("b written\nRebooting..."));
#endif
            ESP.restart();
        }
        else
        {
            Update.printError(Serial1);
        }
    }
}

void setupGUI()
{
    ESPUI.jsonUpdateDocumentSize = 2000; // This is the default, and this value is not affected by the amount of widgets
    ESPUI.jsonInitialDocumentSize = 10000; // Default is 8000. Increased as there are a lot of widgets causing display to not work on newer versions of ESPUI library
 
    ESPUI.begin(DEVICE_FULL_NAME); // It is important that ESPUI.begin(...) is called first so that ESPUI.server is initalized

    ESPUI.server->on("/ota", 
        HTTP_POST, 
        [](AsyncWebServerRequest* request) { request->send(200); }, 
        handleOTAUpload);

    ESPUI.server->on("/ota", 
        HTTP_GET, 
        [](AsyncWebServerRequest* request) {
            AsyncWebServerResponse* response = request->beginResponse_P(200, F("text/html"), OTA_INDEX);
            request->send(response);
        }
    );
}

void SaveButtontxt(Control *sender, int type) {
  //check if already writing EEPRON (double button click)
  if (isWrittingEEPROM) return;

  switch (type) {
  case B_DOWN:
#if defined(DEBUG) || defined(GUI_DEBUG)
    Serial.println(F("\nSave Button Pressed"));
#endif

    WriteConfigToEEPROM();
    ESPUI.updateControlValue(savestatustxt , "Changes Saved");
    break;
  }
}

void RebootButtontxt(Control *sender, int type) {
  switch (type) {
  case B_DOWN:
#if defined(DEBUG) || defined(GUI_DEBUG)
    Serial.println(F("Rebooting"));
#endif
    ESP.restart();
    break;
  }
}

#ifdef ENABLE_HA_FACTORY_RESET_FUNCTIONS
void FactoryResetButtontxt(Control *sender, int type) {
  switch (type) {
  case B_DOWN:
#if defined(DEBUG) || defined(GUI_DEBUG)
    Serial.println(F("Resetging Flash and Rebooting"));
#endif
    FactoryResetSettings();
    ESP.restart();
    break;
  }
}
#endif

void OverVoltDisttxt(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
#endif
}

void OverVoltRecontxt(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
#endif
}

void EQChargeVolttxt(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
#endif
}

void BoostChargeVolttxt(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
#endif
}

void FloatChargeVolttxt(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
#endif
}

void BoostReconChargeVolttxt(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
#endif
}

void BatteryChargePercenttxt(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
#endif
}

void ChargeLimitVolttxt(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
#endif
}

void DischargeLimitVolttxt(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
#endif
}

void LowVoltDisconnecttxt(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
#endif
}

void LowVoltReconnecttxt(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
#endif
}

void UnderVoltWarningVolttxt(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
#endif
}

void UnderVoltReconnectVolttxt(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
#endif
}

void BatteryDischargePercenttxt(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
#endif
}

void BoostDurationtxt(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
#endif
}

void EQDurationtxt(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
#endif
}

void BatteryCapactitytxt(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
#endif
}

void DEVICEIDtxt(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
#endif
  myConfig.Device_ID = atoi ( (sender->value).c_str() );
  ESPUI.updateControlValue(savestatustxt , "Changes Unsaved");
}

void DEVICEBAUDtxt(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
  Serial.begin(myConfig.Device_BAUD);
#endif
  myConfig.Device_BAUD = atoi ( (sender->value).c_str() );
  Serial.begin(myConfig.Device_BAUD);
  ESPUI.updateControlValue(savestatustxt , "Changes Unsaved");
}

void BatteryTypeList(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
#endif
}

void ChargingModeList(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
#endif
}

void RatedVoltagelvlList(Control *sender, int type) {
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(F("Text: ID: "));
  Serial.print(sender->id);
  Serial.print(F(", Value: "));
  Serial.println(sender->value);
#endif
}

void LoadSwitch(Control *sender, int value) {
  switch (value) {
  case S_ACTIVE:
    loadState = true;
    switch_load = 1;
    break;

  case S_INACTIVE:
    loadState = false;
    switch_load = 1;
    break;
  }
  
#if defined(DEBUG) || defined(GUI_DEBUG)
  Serial.print(value == S_ACTIVE ? F("Active: ") : F("Inactive "));
  Serial.println(sender->id);
#endif
}