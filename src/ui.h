#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "config.h"
#include "employee.h"
#include "rtc.h"

struct CardZone {
  int16_t xMin = 0;
  int16_t xMax = 0;
  int16_t yMin = 0;
  int16_t yMax = 0;
  int employeeIndex = -1;
};

class UIMarquee {
  const DisplayConfig& cfg;
public:
  UIMarquee(const DisplayConfig& config);
  unsigned long computeFwdMs(int overflow, int maxWidth) const;
  int position(int textWidth, int maxWidth, unsigned long nowMs, unsigned long maxFwdMs) const;
};

class UIManager {
  Adafruit_ILI9341& tft;
  const DisplayConfig& cfg;
  UIMarquee marquee;
  
  CardZone zones[10];
  int zoneCount = 0;

  void drawHeader(bool rtcOk, const RtcDateTime* now, bool wifiOk, int rssi);
  void drawWifiIcon(int x, int y, int rssi, bool connected);
  void drawClockIcon(int x, int y, bool rtcConnected);
  
public:
  UIManager(Adafruit_ILI9341& display, const DisplayConfig& config);

  void begin();

  void bootScreen(const char* status, const char* ssid);
  void wifiProgress(int attempt, int max);
  void wifiResult(bool ok, const char* ip, int rssi);

  void renderNormal(bool rtcOk, const RtcDateTime* now, EmployeeStore& store,
                    unsigned long currentMs, bool wifiOk, int rssi);
                    
  void renderDebug(bool rtcOk, bool wifiOk, int rssi,
                   const String& ssid, const String& ip);

  void handleTouch(int16_t x, int16_t y, EmployeeStore& store);
};

#endif
