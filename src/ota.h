// Arduino OTA stuff

BearSSL::PublicKey signPubKey(signing_pubkey);
BearSSL::HashSHA256 hash;
BearSSL::SigningVerifier sign(&signPubKey);

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

void ota_h_init() {
    // char hostname[32];
    // uint8_t mac[6];
    // WiFi.macAddress(mac);
    // sprintf(hostname, "van-%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    Update.installSignature(&hash, &sign);

    ArduinoOTA.onStart(arduinoota_onstartup);
    ArduinoOTA.onEnd(arduinoota_onend);
    ArduinoOTA.onError(arduinoota_onerror);
    ArduinoOTA.setPassword(ota_password);
    ArduinoOTA.setHostname(hostname);
    ArduinoOTA.setPort(ota_port);
    ArduinoOTA.begin();
}