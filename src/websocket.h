
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
uint8_t text2int(String input, uint8_t posit) {
    uint8_t output;
    switch (input.charAt(posit)) {
        case '1':
            output = 16;
            break;
        case '2':
            output = 32;
            break;
        case '3':
            output = 48;
            break;
        case '4':
            output = 64;
            break;
        case '5':
            output = 80;
            break;
        case '6':
            output = 96;
            break;
        case '7':
            output = 112;
            break;
        case '8':
            output = 128;
            break;
        case '9':
            output = 144;
            break;
        case 'A':
        case 'a':
            output = 160;
            break;
        case 'B':
        case 'b':
            output = 176;
            break;
        case 'C':
        case 'c':
            output = 192;
            break;
        case 'D':
        case 'd':
            output = 208;
            break;
        case 'E':
        case 'e':
            output = 224;
            break;
        case 'F':
        case 'f':
            output = 240;
            break;
        default:
            output = 0;
            break;
    }
    switch (input.charAt(posit + 1)) {
        case '1':
            output += 1;
            break;
        case '2':
            output += 2;
            break;
        case '3':
            output += 3;
            break;
        case '4':
            output += 4;
            break;
        case '5':
            output += 5;
            break;
        case '6':
            output += 6;
            break;
        case '7':
            output += 7;
            break;
        case '8':
            output += 8;
            break;
        case '9':
            output += 9;
            break;
        case 'A':
        case 'a':
            output += 10;
            break;
        case 'B':
        case 'b':
            output += 11;
            break;
        case 'C':
        case 'c':
            output += 12;
            break;
        case 'D':
        case 'd':
            output += 13;
            break;
        case 'E':
        case 'e':
            output += 14;
            break;
        case 'F':
        case 'f':
            output += 15;
            break;
        default:
            output += 0;
            break;
    }
    Serial.printf("Converted %c%c, got %u", input.charAt(posit), input.charAt(posit + 1), output);
    return output;
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
                        bssid[0] = text2int(req_bssid, 0);
                        bssid[1] = text2int(req_bssid, 3);
                        bssid[2] = text2int(req_bssid, 6);
                        bssid[3] = text2int(req_bssid, 9);
                        bssid[4] = text2int(req_bssid, 12);
                        bssid[5] = text2int(req_bssid, 15);
                        if ((bssid[0] + bssid[1] + bssid[2] + bssid[3] + bssid[4] + bssid[5]) > 0) bssid_valid = true;
                    }
                    Serial.printf("Got set SSID:%s %u, BSSID:%s %u, PASSWORD:%s %u\n", req_ssid.c_str(), req_ssid.length(), req_bssid.c_str(), req_bssid.length(), req_pass.c_str(), req_pass.length());
                    Serial.printf("SSID:\"%s\" PASSWORD:\"%s\" bssid_set:%u bssid:%02X%02X:%02X%02X:%02X%02X\n", req_ssid.c_str(), req_pass.c_str(), bssid_valid, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
                    if (bssid_valid)
                        WiFi.begin(req_ssid.c_str(), req_pass.c_str(), -1, bssid, true);
                    else
                        WiFi.begin(req_ssid.c_str(), req_pass.c_str());
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

void websocket_h_init() {
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
}