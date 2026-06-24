#include "app.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WebServer.h>
#include <ArduinoJson.h>

App::App()
  : gfx(U8G2_R0, cfg.pinClk, cfg.pinData, cfg.pinCs, cfg.pinReset),
    rtc(cfg.rtcData, cfg.rtcClk, cfg.rtcRst),
    wifi(WIFI_SSID, WIFI_PASSWORD, cfg.wifiRetryMs, cfg.wifiAttemptMs, cfg.wifiMaxAttempts),
    renderer(gfx, cfg),
    supabase(SUPABASE_URL, SUPABASE_KEY, SupabaseFieldMapping(), SUPABASE_FETCH_INTERVAL),
    server(nullptr) {}

void App::setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);
  Serial.println("\r\n=== ESP32 OJT Display Sign ===");

  server = new WebServer(80);

  renderer.begin();
  wifi.connect(renderer);

  bool rtcOk = rtc.begin();
  Serial.println(rtcOk ? "RTC OK" : "WARNING: RTC not responding — check wiring / battery");

  startServer();
  fetchEmployees(millis());
}

void App::loop() {
  server->handleClient();

  unsigned long now = millis();

  wifi.poll();
  fetchEmployees(now);
  handleSerial();
  runDisplay(now);
}

// ── Private ─────────────────────────────────────────────────────────────────

void App::startServer() {
  server->on("/status", HTTP_GET, [this]() { handleStatus(); });
  server->on("/debug", HTTP_GET, [this]() { handleToggleDebug(); });
  server->begin();
}

void App::handleStatus() {
  bool rtcOk = rtc.isValid();

#if ARDUINOJSON_VERSION_MAJOR >= 7
  JsonDocument doc;
#else
  StaticJsonDocument<256> doc;
#endif

  doc["device"] = "ESP32 OJT Display Sign";
  doc["wifi_rssi"] = wifi.rssi();
  doc["employees_loaded"] = employees.size();
  doc["debug_mode_active"] = debugMode;
  doc["rtc_status"] = rtcOk ? "CONNECTED" : "DISCONNECTED";

  String response;
  serializeJson(doc, response);
  server->send(200, "application/json", response);
}

void App::handleToggleDebug() {
  debugMode = !debugMode;
  char buf[64];
  snprintf(buf, sizeof(buf), "{\"status\":\"success\",\"debug_mode\":\"%s\"}",
           debugMode ? "ENABLED" : "DISABLED");
  server->send(200, "application/json", buf);
  Serial.printf("Debug Toggled via HTTP: %s\r\n", debugMode ? "ENABLED" : "DISABLED");
}

void App::handleSerial() {
  if (Serial.available() <= 0) return;

  String input = Serial.readStringUntil('\n');
  input.trim();

  if (input.equalsIgnoreCase("debug") || input.equalsIgnoreCase("d")) {
    debugMode = !debugMode;
    Serial.printf("Debug Mode: %s\r\n", debugMode ? "ACTIVE" : "INACTIVE");
  }
}

void App::fetchEmployees(unsigned long now) {
  if (!supabase.shouldFetch(now)) return;
  supabase.markFetched(now);
  supabase.fetch(employees, MAX_EMPLOYEES);
}

void App::runDisplay(unsigned long now) {
  if (now - lastRefresh < cfg.refreshMs) return;
  lastRefresh = now;

  bool rtcOk = rtc.isValid();
  bool wifiOk = wifi.isConnected();
  int rssi = wifiOk ? wifi.rssi() : 0;

  if (debugMode) {
    renderer.renderDebug(rtcOk, wifiOk, rssi, WIFI_SSID, wifi.ip());
    return;
  }

  RtcDateTime rtcNow(2024, 1, 1, 0, 0, 0);
  if (rtcOk) rtcNow = rtc.now();

  String lines[3] = {"Loading...", "Waiting for data", ""};

  if (!employees.empty()) {
    int matches[MAX_EMPLOYEES];
    int matchCount = 0;

    if (rtcOk) {
      uint8_t today = rtcNow.Day();
      uint16_t nowMin = rtcNow.Hour() * 60 + rtcNow.Minute();
      matchCount = employees.collectActive(today, nowMin, matches, MAX_EMPLOYEES);
    }

    if (matchCount > 0) {
      int idx = matches[(now / cfg.rotationMs) % matchCount];
      const Employee& e = employees.get(idx);
      for (int i = 0; i < 3; i++) lines[i] = e.lines[i];
    } else {
      lines[0] = "No one scheduled";
      lines[1] = "";
      lines[2] = "";
    }
  }

  renderer.renderNormal(rtcOk, rtcOk ? &rtcNow : nullptr, lines, now, wifiOk, rssi, WIFI_SSID);
}
