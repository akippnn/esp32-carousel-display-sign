#include "app.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WebServer.h>
#include <ArduinoJson.h>

App::App()
  : spiBus(VSPI),
    display(cfg, &spiBus),
    touch(cfg),
    ui(display.getTft(), cfg),
    rtc(cfg.rtcData, cfg.rtcClk, cfg.rtcRst),
    wifi(WIFI_SSID, WIFI_PASSWORD, cfg.wifiRetryMs, cfg.wifiAttemptMs, cfg.wifiMaxAttempts),
    supabase(SUPABASE_URL, SUPABASE_KEY, SupabaseFieldMapping(), SUPABASE_FETCH_INTERVAL),
    server(nullptr) {}

void App::setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);
  Serial.println("\r\n=== ESP32 OJT Display Sign ===");

  server = new WebServer(80);

  // Initialize Custom Hardware SPI via GPIO matrix routing
  spiBus.begin(cfg.pinClk, cfg.pinMiso, cfg.pinMosi, -1);

  // Initialize display, touch, and UI components
  display.begin();
  touch.begin(spiBus);
  ui.begin();

  wifi.connect(ui);

  bool rtcOk = rtc.begin();
  Serial.println(rtcOk ? "RTC OK" : "WARNING: RTC not responding — check wiring / battery");

  startServer();
  fetchEmployees(millis());
}

void App::loop() {
  server->handleClient();

  unsigned long now = millis();

  // Non-blocking touch polling
  if (touch.poll()) {
    TouchPoint pt = touch.getTouchPoint();
    if (pt.touched) {
      ui.handleTouch(pt.x, pt.y, employees);
      lastRefresh = 0; // Force immediate screen refresh
    }
  }

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
  if (supabase.fetch(employees, MAX_EMPLOYEES)) {
    supabase.markFetched(now);
    Serial.println("Employee data updated successfully");
  } else {
    supabase.markFailed(now);
    Serial.println("Supabase fetch failed, will retry soon");
  }
}

void App::runDisplay(unsigned long now) {
  if (now - lastRefresh < cfg.refreshMs) return;
  lastRefresh = now;

  bool rtcOk = rtc.isValid();
  bool wifiOk = wifi.isConnected();
  int rssi = wifiOk ? wifi.rssi() : 0;

  if (debugMode) {
    ui.renderDebug(rtcOk, wifiOk, rssi, WIFI_SSID, wifi.ip());
    return;
  }

  RtcDateTime rtcNow(2024, 1, 1, 0, 0, 0);
  if (rtcOk) rtcNow = rtc.now();

  ui.renderNormal(rtcOk, rtcOk ? &rtcNow : nullptr, employees, now, wifiOk, rssi);
}
