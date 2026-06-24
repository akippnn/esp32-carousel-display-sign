#include "app.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#if DATABASE_PROVIDER_FIREBASE
  #ifndef FIREBASE_URL
    #error "FIREBASE_URL not defined. Add FIREBASE_URL=your_url to .env"
  #endif
  #ifndef FIREBASE_CLIENT_EMAIL
    #error "FIREBASE_CLIENT_EMAIL not defined. Add FIREBASE_CLIENT_EMAIL=your_email to .env"
  #endif
  #ifndef FIREBASE_PRIVATE_KEY
    #error "FIREBASE_PRIVATE_KEY not defined. Add FIREBASE_PRIVATE_KEY=your_key to .env"
  #endif
#else
  #ifndef SUPABASE_URL
    #error "SUPABASE_URL not defined. Add SUPABASE_URL=your_url to .env"
  #endif
  #ifndef SUPABASE_KEY
    #error "SUPABASE_KEY not defined. Add SUPABASE_KEY=your_key to .env"
  #endif
#endif

App::App()
  : spiBus(VSPI),
    display(cfg, &spiBus),
    touch(cfg),
    ui(display.getTft(), cfg),
    rtc(cfg.rtcData, cfg.rtcClk, cfg.rtcRst),
    wifi(WIFI_SSID, WIFI_PASSWORD, cfg.wifiRetryMs, cfg.wifiAttemptMs, cfg.wifiMaxAttempts),
#if DATABASE_PROVIDER_FIREBASE
    dbClient(FIREBASE_URL, FIREBASE_CLIENT_EMAIL, FIREBASE_PRIVATE_KEY, SupabaseFieldMapping(), SUPABASE_FETCH_INTERVAL),
#else
    dbClient(SUPABASE_URL, SUPABASE_KEY, SupabaseFieldMapping(), SUPABASE_FETCH_INTERVAL),
#endif
    server(nullptr) {}

void App::setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);
  Serial.println("\r\n=== ESP32 OJT Display Sign ===");

  server = new WebServer(80);

  // Deselect SPI devices on startup to prevent bus contention
  pinMode(cfg.pinTftCs, OUTPUT);
  digitalWrite(cfg.pinTftCs, HIGH);
  pinMode(cfg.pinTouchCs, OUTPUT);
  digitalWrite(cfg.pinTouchCs, HIGH);

  // Initialize Custom Hardware SPI via GPIO matrix routing
  spiBus.begin(cfg.pinClk, cfg.pinMiso, cfg.pinMosi, -1);

  // Initialize display, touch, and UI components
  display.begin();
#if TOUCH_SCREEN_ENABLED
  touch.begin(spiBus);
#endif
  ui.begin();

  // Load custom Wi-Fi credentials from Preferences if saved
  Preferences prefs;
  prefs.begin("wifi-config", true);
  String savedSsid = prefs.getString("ssid", WIFI_SSID);
  String savedPass = prefs.getString("password", WIFI_PASSWORD);
  prefs.end();

  wifi.updateCredentials(savedSsid, savedPass);
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
#if TOUCH_SCREEN_ENABLED
  if (touch.poll()) {
    TouchPoint pt = touch.getTouchPoint();
    if (pt.touched) {
      ui.handleTouch(pt.x, pt.y, employees);
      lastRefresh = 0; // Force immediate screen refresh
    }
  }

  // Check if UI requested a Wi-Fi reconnection (credentials submitted)
  if (ui.shouldReconnectWifi) {
    ui.clearReconnectFlag();
    String newSsid = ui.getNewSsid();
    String newPass = ui.getNewPassword();
    Serial.printf("App: Reconnecting to new Wi-Fi: %s...\r\n", newSsid.c_str());
    wifi.updateCredentials(newSsid, newPass);
    wifi.connect(ui);
    lastRefresh = 0; // Force refresh normal screen
  }
#endif

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
  if (!dbClient.shouldFetch(now)) return;

  uint32_t nowEpoch = 0;
  if (rtc.isValid()) {
    nowEpoch = rtc.now().Unix32Time();
  }

  if (dbClient.fetch(employees, MAX_EMPLOYEES, nowEpoch)) {
    dbClient.markFetched(now);
    Serial.println("Employee data updated successfully");
  } else {
    dbClient.markFailed(now);
#if DATABASE_PROVIDER_FIREBASE
    Serial.println("Firebase fetch failed, will retry soon");
#else
    Serial.println("Supabase fetch failed, will retry soon");
#endif
  }
}

void App::runDisplay(unsigned long now) {
  if (now - lastRefresh < cfg.refreshMs) return;
  lastRefresh = now;

  bool rtcOk = rtc.isValid();
  bool wifiOk = wifi.isConnected();
  int rssi = wifiOk ? wifi.rssi() : 0;

  RtcDateTime rtcNow(2024, 1, 1, 0, 0, 0);
  if (rtcOk) rtcNow = rtc.now();

  // If debugMode is toggled, update UI mode dynamically
  if (debugMode) {
    ui.setMode(SCREEN_DEBUG);
  } else if (ui.getMode() == SCREEN_DEBUG) {
    ui.setMode(SCREEN_NORMAL);
  }

  // Call the main UI router rendering function
  ui.render(rtcOk, rtcOk ? &rtcNow : nullptr, employees, now, wifiOk, rssi);
}
