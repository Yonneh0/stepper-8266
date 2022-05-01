void ota_onUpdate() {
    HTTPUpload& upload = webServer.upload();

    Serial.printf("OTA onUpdate Status %s, filename:%s, name:%s, totalSize:%u, currentSize:%u, contentLength:%u\n",
                  ((upload.status == UPLOAD_FILE_START) ? "UPLOAD_FILE_START" : (upload.status == UPLOAD_FILE_WRITE) ? "UPLOAD_FILE_WRITE"
                                                                            : (upload.status == UPLOAD_FILE_END)     ? "UPLOAD_FILE_END"
                                                                                                                     : "UPLOAD_FILE_ABORTED"),
                  upload.filename.c_str(), upload.name.c_str(), upload.totalSize, upload.currentSize, upload.contentLength);
    switch (upload.status) {
        case UPLOAD_FILE_START: {
            Serial.setDebugOutput(true);
            WiFiUDP::stopAll();
            Serial.printf("Update: %s\n", upload.filename.c_str());
            uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
            if (!Update.begin(maxSketchSpace)) {  // start with max available size
                Update.printError(Serial);
            }
            break;
        }
        case UPLOAD_FILE_WRITE:
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
            break;
        case UPLOAD_FILE_END:
            if (Update.end(true)) {  // true to set the size to the current progress
                Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
            Serial.setDebugOutput(false);
            break;
        case UPLOAD_FILE_ABORTED:
            if (Update.end(false)) {  // true to set the size to the current progress
                Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
            break;
    }
    yield();
}
void ota_onUpdate_ufn() {
    webServer.sendHeader("Connection", "close");
    webServer.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
}
