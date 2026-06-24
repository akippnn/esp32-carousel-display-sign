#include "config.h"
#include "display.h"
#include "supabase.h"
#include "api.h"
#include <esp_wifi.h>

static void connectWiFi() {
  const int maxAttempts = 60;
  const char* spinner = "|/-\\";

  u8g2.clearBuffer();
  u8g2.drawFrame(0, 0, 128, 64);
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(10, 16, "Connecting to WiFi...");
  u8g2.drawStr(10, 30, ssid);
  u8g2.sendBuffer();

  if (password == nullptr || strlen(password) == 0) {
    WiFi.begin(ssid);
  } else {
    WiFi.begin(ssid, password);
  }

  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < maxAttempts) {
    delay(500);
    attempt++;

    char buf[16];
    snprintf(buf, sizeof(buf), "%c  %ds", spinner[attempt % 4], attempt / 2);

    u8g2.setDrawColor(0);
    u8g2.drawBox(10, 42, 108, 12);
    u8g2.setDrawColor(1);
    u8g2.drawStr(10, 52, buf);
    u8g2.sendBuffer();

    Serial.print(".");
  }

  u8g2.clearBuffer();
  u8g2.drawFrame(0, 0, 128, 64);
  if (WiFi.status() == WL_CONNECTED) {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(10, 20, "WiFi Connected!");
    u8g2.setCursor(10, 34);
    u8g2.print(WiFi.localIP().toString().c_str());
    char rssiBuf[32];
    snprintf(rssiBuf, sizeof(rssiBuf), "RSSI: %d dBm", WiFi.RSSI());
    u8g2.drawStr(10, 48, rssiBuf);

    esp_wifi_set_ps(WIFI_PS_NONE);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    Serial.println("\r\nRF: modem active, 19.5dBm TX");
    Serial.printf("Connected to %s, IP: %s\r\n", ssid, WiFi.localIP().toString().c_str());
  } else {
    u8g2.setFont(u8g2_font_helvB10_tf);
    u8g2.drawStr(6, 28, "WiFi Offline");
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(10, 44, "Will keep retrying...");
  }
  u8g2.sendBuffer();
  delay(1500);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\r\n=== OPTIMIZED CAROUSEL SYSTEM STARTING ===");

  u8g2.begin();

  connectWiFi();

  Rtc.Begin();
  delay(50);

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  bool rtcValid = Rtc.IsDateTimeValid();

  if (!rtcValid) {
    Serial.println("RTC not valid, setting compiled time...");
    Rtc.SetDateTime(compiled);
    delay(10);
    rtcValid = Rtc.IsDateTimeValid();
  }

  if (Rtc.GetIsWriteProtected()) {
    Rtc.SetIsWriteProtected(false);
  }

  if (!Rtc.GetIsRunning()) {
    Rtc.SetIsRunning(true);
    Serial.println("RTC oscillator restarted");
  }

  if (!rtcValid) {
    Serial.println("WARNING: RTC not responding — check wiring / battery");
  } else {
    Serial.println("RTC OK");
  }

  server.on("/status", HTTP_GET, handleGetStatus);
  server.on("/debug", HTTP_GET, handleToggleDebug);
  server.begin();

  fetchSupabaseData();
}

static unsigned long lastWifiRetry = 0;

void loop() {
  server.handleClient();

  unsigned long currentMillis = millis();

  if (WiFi.status() != WL_CONNECTED && currentMillis - lastWifiRetry >= 10000) {
    lastWifiRetry = currentMillis;
    Serial.println("WiFi dropped — reconnecting...");
    WiFi.disconnect();
    if (password == nullptr || strlen(password) == 0) {
      WiFi.begin(ssid);
    } else {
      WiFi.begin(ssid, password);
    }
  }

  if (currentMillis - lastSupabaseFetch >= supabaseFetchInterval) {
    lastSupabaseFetch = currentMillis;
    fetchSupabaseData();
  }

  if (Serial.available() > 0) {
    String serialInput = Serial.readStringUntil('\n');
    serialInput.trim();
    if (serialInput.equalsIgnoreCase("debug") || serialInput.equalsIgnoreCase("d")) {
      debugMode = !debugMode;
      Serial.printf("Debug Mode Changed via Serial to: %s\r\n", debugMode ? "ACTIVE" : "INACTIVE");
    }
  }

  static unsigned long lastRefresh = 0;

  if (currentMillis - lastRefresh >= 40) {
    lastRefresh = currentMillis;

    bool isRtcConnected = Rtc.IsDateTimeValid();

    String displayLines[3] = {
      "Loading...",
      "Waiting for data",
      ""
    };

    // Filter by current day-of-month and time window
    int matches[maxEmployees];
    int matchCount = 0;
    if (isRtcConnected) {
      RtcDateTime now = Rtc.GetDateTime();
      uint8_t today = now.Day();
      uint16_t nowMin = now.Hour() * 60 + now.Minute();
      for (int i = 0; i < employeeCount; i++) {
        if (employeeScheduleDay[i] == today &&
            nowMin >= employeeStartMinutes[i] &&
            nowMin < employeeEndMinutes[i]) {
          matches[matchCount++] = i;
        }
      }
    }

    if (matchCount > 0) {
      int idx = matches[(currentMillis / 5000) % matchCount];
      displayLines[0] = employeeData[idx][0];
      displayLines[1] = employeeData[idx][1];
      displayLines[2] = employeeData[idx][2];
    } else if (employeeCount > 0) {
      displayLines[0] = "No one scheduled";
      displayLines[1] = "";
      displayLines[2] = "";
    }

    int currentRssi = 0;
    bool isWifiConnected = (WiFi.status() == WL_CONNECTED);
    if (isWifiConnected) {
      currentRssi = WiFi.RSSI();
    }

    u8g2.clearBuffer();

    if (debugMode) {
      u8g2.drawFrame(0, 0, 128, 64);
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(6, 12, "[WiFi Diagnostic]");
      u8g2.drawHLine(4, 15, 120);

      u8g2.setCursor(6, 28);
      u8g2.print("SSID: ");
      u8g2.print(ssid);

      u8g2.setCursor(6, 40);
      u8g2.print("IP  : ");
      u8g2.print(isWifiConnected ? WiFi.localIP().toString().c_str() : "Disconnected");

      u8g2.setCursor(6, 52);
      u8g2.print("RSSI: ");
      if (isWifiConnected) {
        u8g2.print(currentRssi);
        u8g2.print(" dBm | RTC: ");
        u8g2.print(isRtcConnected ? "OK" : "ERR");
      } else {
        u8g2.print("N/A");
      }
    } else {
      u8g2.drawFrame(0, 0, 128, 64);

      drawClockIcon(6, 6, isRtcConnected);
      drawWifiIcon(116, 6, currentRssi, isWifiConnected);

      if (isRtcConnected) {
        RtcDateTime now = Rtc.GetDateTime();
        u8g2.setFont(u8g2_font_6x10_tf);

        char hours[3], minutes[3];
        snprintf(hours, sizeof(hours), "%02u", now.Hour());
        snprintf(minutes, sizeof(minutes), "%02u", now.Minute());

        u8g2.drawStr(16, 13, hours);
        if (now.Second() % 2 == 0) {
          u8g2.drawStr(28, 12, ":");  // colon 1px higher
        }
        u8g2.drawStr(34, 13, minutes);

        static const char* months[] = {
          "Jan", "Feb", "Mar", "Apr", "May", "Jun",
          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };
        char dateStr[8];
        snprintf(dateStr, sizeof(dateStr), "%s %u", months[now.Month() - 1], now.Day());
        u8g2.drawStr(52, 13, dateStr);
      }

      u8g2.drawHLine(4, 17, 120);

      {
        unsigned long maxFwdMs = 0;
        int textWidths[3] = {0, 0, 0};

        for (int i = 0; i < 3; i++) {
          if (displayLines[i].length() == 0) continue;
          u8g2.setFont(i == 0 ? u8g2_font_helvB10_tf : u8g2_font_6x10_tf);
          textWidths[i] = u8g2.getStrWidth(displayLines[i].c_str());
          int overflow = textWidths[i] - 110;
          if (overflow > 0) {
            unsigned long fwd = (unsigned long)overflow * 400 / 110;
            if (fwd < 300) fwd = 300;
            if (fwd > maxFwdMs) maxFwdMs = fwd;
          }
        }

        for (int i = 0; i < 3; i++) {
          if (displayLines[i].length() == 0) continue;
          int y_pos = (i == 0) ? 32 : ((i == 1) ? 46 : 57);
          u8g2.setFont(i == 0 ? u8g2_font_helvB10_tf : u8g2_font_6x10_tf);
          int x = marqueeX(textWidths[i], 110, currentMillis, i, maxFwdMs);
          u8g2.drawStr(x, y_pos, displayLines[i].c_str());
        }
      }
    }

    u8g2.sendBuffer();
  }
}
