#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include "network.h"
#include "display.h"
#include <esp_wifi.h>

WifiManager::WifiManager(const char* s, const char* p, uint16_t retry, uint16_t attempt, uint8_t max)
  : ssid(s), password(p), retryMs(retry), attemptMs(attempt), maxAttempts(max) {}

bool WifiManager::connect(DisplayRenderer& display) {
  display.bootScreen("Connecting to WiFi...", ssid);

  if (password == nullptr || strlen(password) == 0) {
    WiFi.begin(ssid);
  } else {
    WiFi.begin(ssid, password);
  }

  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < maxAttempts) {
    delay(attemptMs);
    attempt++;
    display.wifiProgress(attempt, maxAttempts);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    esp_wifi_set_ps(WIFI_PS_NONE);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    Serial.println("\r\nRF: modem active, 19.5dBm TX");
    Serial.printf("Connected to %s, IP: %s\r\n", ssid, ip().c_str());
  }

  display.wifiResult(isConnected(), ip().c_str(), rssi());
  delay(1500);
  return isConnected();
}

bool WifiManager::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

int WifiManager::rssi() const {
  return WiFi.RSSI();
}

String WifiManager::ip() const {
  return WiFi.localIP().toString();
}

void WifiManager::poll() {
  if (!isConnected() && millis() - lastRetry >= retryMs) {
    lastRetry = millis();
    Serial.println("WiFi dropped — reconnecting...");
    WiFi.disconnect();
    if (password == nullptr || strlen(password) == 0) {
      WiFi.begin(ssid);
    } else {
      WiFi.begin(ssid, password);
    }
  }
}
