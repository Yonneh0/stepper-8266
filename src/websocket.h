
unsigned long wifi_scanStart;

volatile bool webSocket_WiFiScanResultsReady = false;

void WiFi_onScanComplete(int8_t scanCount) {
    Serial.printf("WiFi Scan Complete says %i after %lums\n", scanCount, millis() - wifi_scanStart);
    webSocket_WiFiScanResultsReady = true;  // put a conveluted note on the 'fridge
}

void webSocketCheckResults() {
    if (webSocket_WiFiScanResultsReady) {
        String ssid;
        int32_t rssi;
        uint8_t encryptionType;
        uint8_t *bssid;
        int32_t channel;
        int8_t scanCount = WiFi.scanComplete();
        DynamicJsonDocument doc(4096);

        bool hidden;
        char buf[1024];
        char bssidtxt[18];
        for (int8_t i = 0; i < scanCount; i++) {
            WiFi.getNetworkInfo(i, ssid, encryptionType, rssi, bssid, channel, hidden);
            snprintf(bssidtxt, 18, "%02X:%02X:%02X:%02X:%02X:%02X", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
            doc["ssid"] = ssid;
            doc["rssi"] = rssi;
            doc["bssid"] = bssidtxt;

            serializeJson(doc, buf);
            webSocket.broadcastTXT(buf);
            delay(1);
        }
        webSocket.broadcastTXT("=!-");           // reply, indicating that scan has concluded (technically removes the spinny thingie)
        webSocket_WiFiScanResultsReady = false;  // lick the back of that conveluted post-it, and put it back on the 'fridge
    }
}

uint8_t hex2dec_c(const char *s) {
    uint8_t v, n = 0;
    for (int i = 0; i < 2 && s[i] != '\0'; i++) {
        v = 0;
        if ('a' <= s[i] && s[i] <= 'f') {
            v = s[i] - 97 + 10;
        } else if ('A' <= s[i] && s[i] <= 'F') {
            v = s[i] - 65 + 10;
        } else if ('0' <= s[i] && s[i] <= '9') {
            v = s[i] - 48;
        } else
            break;
        n *= 16;
        n += v;
    }
    return n;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    uint32_t newTicks = 0;
    switch (type) {
        case WStype_CONNECTED: {
            // send nice-to-knows
            String payload = "7" + WiFi.SSID();
            webSocket.sendTXT(num, payload);
            payload = "8" + WiFi.localIP().toString();
            webSocket.sendTXT(num, payload);
            payload = "9" + WiFi.macAddress();
            webSocket.sendTXT(num, payload);
            payload = "-" + String(WiFi.RSSI());
            webSocket.sendTXT(num, payload);
            payload = "6" + String(WiFi.channel());
            webSocket.sendTXT(num, payload);

            // send enabled status
            if (GPIP(STEPPER_RST)) {
                webSocket.sendTXT(num, "e1");
            } else {
                webSocket.sendTXT(num, "e0");
            }
            faultStatus = 1;                   // simulates a fault... really just forces the loop to send current status.
            stepperSetDir(GPIP(STEPPER_DIR));  // set stepper dir to current direction- simulates an event.
        } break;
        case WStype_TEXT:
            newTicks = atoi((const char *)(payload + 1));
            switch ((char)payload[0]) {
                case '!':
                    // Serial.printf("[%u] Got Stepper STOP\n", num);
                    stepperStop();
                    break;
                case '+':
                    // Serial.printf("[%u] Got Stepper Enable\n", num);
                    stepperEnable();
                    break;
                case '-':
                    // Serial.printf("[%u] Got Stepper Disable\n", num);
                    stepperDisable();
                    break;

                case 'c':
                    // Serial.printf("[%u] Got Stepper Const\n", num);
                    stepperMove(StepperMode_Const);
                    break;
                case 'm':
                    // Serial.printf("[%u] Got Stepper Step\n", num);
                    stepperMove(StepperMode_Step);
                    break;
                case 'w':
                    // Serial.printf("[%u] Got Stepper Wipe\n", num);
                    stepperMove(StepperMode_Wipe);
                    break;

                case 'd':
                    // Serial.printf("[%u] Got Stepper Direction %u\n", num, newTicks);
                    stepperSetDir(newTicks);
                    break;
                case 't':
                    // Serial.printf("[%u] Got Stepper set ticks %u\n", num, newTicks);
                    if (newTicks < STEPPER_MAX_SPEED) newTicks = STEPPER_MAX_SPEED;  // see, I know what input protection is!
                    startingTicks = pendingTicks = newTicks;
                    break;
                case 's':
                    // Serial.printf("[%u] Got Stepper %u steps\n", num, newTicks);
                    startingSteps = pendingSteps = newTicks;
                    break;

                case '?':
                    Serial.printf("[%u] Got Network Scan\n", num);
                    wifi_scanStart = millis();
                    WiFi.scanNetworksAsync(WiFi_onScanComplete, true);
                    webSocket.sendTXT(num, "=!+");  // reply, indicating that scan has started (technically starts the spinny thingie, and creates the table)

                    break;
                case '_': {
                    StaticJsonDocument<256> doc;
                    deserializeJson(doc, (payload + 1));
                    String req_ssid = doc["ssid"];
                    String req_bssid = doc["bssid"];
                    String req_pass = doc["password"];
                    uint8_t bssid[6] = {0, 0, 0, 0, 0, 0};
                    bool bssid_valid = false;
                    if ((req_bssid.length() == 17) && (((req_bssid.charAt(2) + req_bssid.charAt(5) + req_bssid.charAt(8) + req_bssid.charAt(11) + req_bssid.charAt(14)) == (0x3A * 5)))) {
                        bssid[0] = hex2dec_c(&req_bssid[0]);
                        bssid[1] = hex2dec_c(&req_bssid[3]);
                        bssid[2] = hex2dec_c(&req_bssid[6]);
                        bssid[3] = hex2dec_c(&req_bssid[9]);
                        bssid[4] = hex2dec_c(&req_bssid[12]);
                        bssid[5] = hex2dec_c(&req_bssid[15]);
                        if ((bssid[0] + bssid[1] + bssid[2] + bssid[3] + bssid[4] + bssid[5]) > 0) bssid_valid = true;
                    }
                    Serial.printf("Got set SSID:%s %u, BSSID:%s %u, PASSWORD:%s %u\n", req_ssid.c_str(), req_ssid.length(), req_bssid.c_str(), req_bssid.length(), req_pass.c_str(), req_pass.length());
                    Serial.printf("SSID:\"%s\" PASSWORD:\"%s\" bssid_set:%u bssid:%02X%02X:%02X%02X:%02X%02X\n", req_ssid.c_str(), req_pass.c_str(), bssid_valid, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
                    if (bssid_valid)
                        WiFi.begin(req_ssid.c_str(), req_pass.c_str(), -1, bssid, true);
                    else
                        WiFi.begin(req_ssid.c_str(), req_pass.c_str(), -1, NULL, true);
                    break;
                }
                default:
                    Serial.printf("[%u] get Unknown Text: %.*s\n", num, length, payload);
                    break;
            }
            break;
        case WStype_DISCONNECTED:
        case WStype_PING:
        case WStype_PONG:
        case WStype_BIN:
        case WStype_ERROR:
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            break;
    }
}
