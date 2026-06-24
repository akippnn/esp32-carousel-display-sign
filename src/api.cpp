#include "config.h"
#include "api.h"
#include <ArduinoJson.h>

void handleGetStatus() {
#if ARDUINOJSON_VERSION_MAJOR >= 7
  JsonDocument doc;
#else
  StaticJsonDocument<256> doc;
#endif

  bool isRtcValid = Rtc.IsDateTimeValid();

  doc["device"] = "ESP32 Carousel Display Sign";
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["employees_loaded"] = employeeCount;
  doc["debug_mode_active"] = debugMode;
  doc["rtc_status"] = isRtcValid ? "CONNECTED" : "DISCONNECTED";

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleToggleDebug() {
  debugMode = !debugMode;
  char responseBuffer[64];
  snprintf(responseBuffer, sizeof(responseBuffer), "{\"status\":\"success\",\"debug_mode\":\"%s\"}",
           debugMode ? "ENABLED" : "DISABLED");
  server.send(200, "application/json", responseBuffer);
  Serial.printf("Debug Diagnostics Toggled via HTTP: %s\r\n", debugMode ? "ENABLED" : "DISABLED");
}
