// Arduino OTA stuff

void arduinoota_onstartup() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
    else  // U_SPIFFS
        type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
}
void arduinoota_onend() {
    Serial.println("\nEnd");
}

void arduinoota_onerror(ota_error_t error) {
    if (error == OTA_AUTH_ERROR) {
        Serial.println("[ArduinoOTA] Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("[ArduinoOTA] Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("[ArduinoOTA] Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("[ArduinoOTA] Receive Failed");
    } else if (error == OTA_END_ERROR) {
        Serial.println("[ArduinoOTA] End Failed");
    }
}
