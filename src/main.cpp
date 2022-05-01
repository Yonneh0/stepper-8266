#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <WebSocketsServer.h>

#include "config.h"
#include "ftpserver.h"
DNSServer dnsServer;
FtpServer ftpSrv;
ESP8266WebServer webServer(80);
WebSocketsServer webSocket = WebSocketsServer(81);
uint16_t ota_port = 13442;

const char* ap_name = "stepper";
const char* ap_pass = "stepperstepper";
const char* hostname = "stepper";
const char* ota_password = "pineapples";

#include "ota.h"
#include "stepper.h"
#include "web.h"
#include "websocket.h"

unsigned long wifiConnectTime;

void WiFi_onStationModeConnected(const WiFiEventStationModeConnected& e) {
    Serial.printf("WiFi STA Connected SSID:\"%s\" CH:%u BSSID:%02X%02X:%02X%02X:%02X%02X\n", e.ssid.c_str(), e.channel, e.bssid[0], e.bssid[1], e.bssid[2], e.bssid[3], e.bssid[4], e.bssid[5]);
}
void WiFi_onStationModeDHCPTimeout() {
    Serial.printf("WiFi STA DHCP Timeout\n");
}

void WiFi_onStationModeDisconnected(const WiFiEventStationModeDisconnected& e) {
    Serial.printf("WiFi STA Disconnected SSID:\"%s\" BSSID:%02X%02X:%02X%02X:%02X%02X reason:%u\n", e.ssid.c_str(), e.bssid[0], e.bssid[1], e.bssid[2], e.bssid[3], e.bssid[4], e.bssid[5], e.reason);
}
void WiFi_onStationModeGotIP(const WiFiEventStationModeGotIP& e) {
    Serial.printf("WiFi STA Got IP:%s NM:%s GW:%s after %lums\n", e.ip.toString().c_str(), e.mask.toString().c_str(), e.gw.toString().c_str(), millis() - wifiConnectTime);
}

void WiFi_onSoftAPModeStationConnected(const WiFiEventSoftAPModeStationConnected& e) {
    Serial.printf("WiFi Soft AP Station Connected: AID:%u MAC:%02X%02X:%02X%02X:%02X%02X\n", e.aid, e.mac[0], e.mac[1], e.mac[2], e.mac[3], e.mac[4], e.mac[5]);
}

void WiFi_onSoftAPModeStationDisconnected(const WiFiEventSoftAPModeStationDisconnected& e) {
    Serial.printf("WiFi Soft AP Station Disconnected: AID:%u MAC:%02X%02X:%02X%02X:%02X%02X\n", e.aid, e.mac[0], e.mac[1], e.mac[2], e.mac[3], e.mac[4], e.mac[5]);
}

void WiFi_onWiFiModeChange(const WiFiEventModeChange& e) {
    Serial.printf("WiFi Mode Change: %u -> %u\n", (e.oldMode & 0x03), (e.newMode & 0x03));
}

WiFiEventHandler weh[7];

void setup() {
    stepper_h_init();

    Serial.begin(115200);
    while (!Serial) yield();
    Serial.printf("\n\n================================================================\n");
    Serial.printf("ESP8266 %08X STA MAC:%s, AP MAC:%s\n", ESP.getChipId(), WiFi.macAddress().c_str(), WiFi.softAPmacAddress().c_str());
    Serial.printf("Free Sketch Space:%u, Sketch Size:%u, Sketch MD5:%s, SDK:%s\n", ESP.getFreeSketchSpace(), ESP.getSketchSize(), ESP.getSketchMD5().c_str(), ESP.getSdkVersion());
    uint32_t realSize = ESP.getFlashChipRealSize();
    uint32_t ideSize = ESP.getFlashChipSize();
    FlashMode_t ideMode = ESP.getFlashChipMode();

    Serial.printf("Real Flash ID:%08X, size:%ub. IDE size:%ub, %uHz, %s\n", ESP.getFlashChipId(), realSize, ideSize, ESP.getFlashChipSpeed(),
                  (ideMode == FM_QIO    ? "QIO"
                   : ideMode == FM_QOUT ? "QOUT"
                   : ideMode == FM_DIO  ? "DIO"
                   : ideMode == FM_DOUT ? "DOUT"
                                        : "UNKNOWN"));

    if (ideSize != realSize) {
        Serial.println("Flash Chip configuration wrong!\n");
    }
    LittleFS.begin();

    weh[0] = WiFi.onStationModeConnected(WiFi_onStationModeConnected);
    weh[1] = WiFi.onStationModeDHCPTimeout(WiFi_onStationModeDHCPTimeout);
    weh[2] = WiFi.onStationModeDisconnected(WiFi_onStationModeDisconnected);
    weh[3] = WiFi.onStationModeGotIP(WiFi_onStationModeGotIP);
    weh[4] = WiFi.onSoftAPModeStationConnected(WiFi_onSoftAPModeStationConnected);
    weh[5] = WiFi.onSoftAPModeStationDisconnected(WiFi_onSoftAPModeStationDisconnected);
    weh[6] = WiFi.onWiFiModeChange(WiFi_onWiFiModeChange);

    struct station_config c;
    wifi_station_get_config_default(&c);
    Serial.printf("WiFi STA DEFAULT CONFIG: SSID:\"%s\" PASSWORD:\"%s\" bssid_set:%u bssid:%02X%02X:%02X%02X:%02X%02X threshold rssi:%i auth mode:%i\n", c.ssid, c.password, c.bssid_set, c.bssid[0], c.bssid[1], c.bssid[2], c.bssid[3], c.bssid[4], c.bssid[5], c.threshold.rssi, c.threshold.authmode);

    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.mode(WIFI_AP_STA);
    dnsServer.start(53, "*", IPAddress({10, 255, 255, 1}));  // DNS spoofing (Only HTTP)
    WiFi.softAPConfig(IPAddress({10, 255, 255, 1}), IPAddress({10, 255, 255, 1}), IPAddress({255, 255, 255, 0}));

    wifiConnectTime = millis();  // should be basically 0.... but...
    WiFi.begin();
    yield();
    Serial.printf("Connecting to WiFi...\n");
    int8_t connectResult = WiFi.waitForConnectResult(30000);
    if (connectResult != WL_CONNECTED) {  // Wait for the Wi-Fi to connect
        Serial.printf("Connect Result was %i, activating AP \"stepper\"\n", connectResult);
        if (!WiFi.softAP(ap_name, ap_pass)) {
            yield();
            Serial.printf("WiFi AP Setup Failure!\n");
        } else {
            yield();
            Serial.printf("AP SSID \"%s\" password \"%s\" started on %s [%s]...\n", WiFi.softAPSSID().c_str(), WiFi.softAPPSK().c_str(), WiFi.softAPIP().toString().c_str(), WiFi.softAPmacAddress().c_str());
        }
    }

    webServer.on("/update", HTTP_POST, ota_onUpdate_ufn, ota_onUpdate);
    webServer.serveStatic("/", LittleFS, "/", "max-age=31536000");
    webServer.begin();

    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    ArduinoOTA.onStart(arduinoota_onstartup);
    ArduinoOTA.onEnd(arduinoota_onend);
    ArduinoOTA.onError(arduinoota_onerror);
    ArduinoOTA.setPassword(ota_password);
    ArduinoOTA.setHostname(hostname);
    ArduinoOTA.setPort(ota_port);
    ArduinoOTA.begin();

    ftpSrv.begin("admin", "minad");

    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);
    MDNS.addService("ftp", "tcp", 21);

    Serial.printf("Ready: http://%s.local/\n", hostname);
    Serial.printf("       ftp://admin:minad@%s.local/\n", hostname);
    Serial.printf("       http://%s/\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("       http://%s/\n", WiFi.localIP().toString().c_str());
}

void loop() {
    yield();
    webSocket.loop();
    webServer.handleClient();
    dnsServer.processNextRequest();
    MDNS.update();
    ftpSrv.handleFTP();
    ArduinoOTA.handle();
    stepperCheckFault();
    webSocketCheckResults();
}