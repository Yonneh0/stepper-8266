
#include "web/cw.png.h"
#include "web/dark.min.js.h"
#include "web/favicon.ico.h"
#include "web/index.htm.h"
#include "web/minstyle.io.css.h"
#include "web/minstyle.io.css.map.h"

void serverSendCache(bool setCache) {
    server.sendHeader("Content-Encoding", "gzip");
    if (setCache)
        server.sendHeader("Cache-Control", "max-age=31536000");
    else
        server.sendHeader("Cache-Control", "max-age=0");
}

void web_h_init() {
    server.on("/", []() { serverSendCache(false); server.send(200, "text/html", index_htm_gz, sizeof(index_htm_gz)); });
    server.on("/favicon.ico", []() { serverSendCache(true); server.send(200, "image/vnd.microsoft.icon", favicon_ico_gz, sizeof(favicon_ico_gz)); });
    server.on("/dark.min.js", []() { serverSendCache(true); server.send(200, "text/javascript", dark_min_js_gz, sizeof(dark_min_js_gz)); });
    server.on("/minstyle.io.css", []() { serverSendCache(true); server.send(200, "text/css", minstyle_io_css_gz, sizeof(minstyle_io_css_gz)); });
    server.on("/minstyle.io.css.map", []() { serverSendCache(true); server.send(200, "application/json", minstyle_io_css_map_gz, sizeof(minstyle_io_css_map_gz)); });
    server.on("/cw.png", []() { serverSendCache(true); server.send(200, "text/css", cw_png_gz, sizeof(cw_png_gz)); });
    server.on(
        "/update", HTTP_POST, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart(); }, []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.setDebugOutput(true);
            WiFiUDP::stopAll();
            Serial.printf("Update: %s\n", upload.filename.c_str());
            uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
            if (!Update.begin(maxSketchSpace)) {  // start with max available size
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {  // true to set the size to the current progress
                Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
            Serial.setDebugOutput(false);
        }
      yield(); });
    server.begin();
}