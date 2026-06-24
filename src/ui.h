#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "config.h"
#include "employee.h"
#include "rtc.h"

enum ScreenMode {
  SCREEN_NORMAL,
  SCREEN_DEBUG,
  SCREEN_WIFI_SETTINGS
};

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

  // --- Rendering Cache (Anti-Flicker) ---
  int lastActiveCount = -1;
  int lastActiveIndices[4] = {-1, -1, -1, -1};
  bool lastCheckedIn[4] = {false, false, false, false};
  int lastOffsets[12]; // Up to 4 cards * 3 lines
  unsigned long lastClockTime = 0;
  bool forceRedraw = true;

  // --- WiFi Settings Screen & Keyboard ---
  ScreenMode currentMode = SCREEN_NORMAL;

#if TOUCH_SCREEN_ENABLED
  String wifiInputSsid;
  String wifiInputPass;
  bool keyboardShift = false;
  bool keyboardSymbols = false;
  int activeInputField = 0; // 0 = SSID, 1 = Password
  
  String scannedSsids[5];
  int scannedCount = 0;
  bool isScanning = false;
  unsigned long scanStartTime = 0;
#endif

  void drawHeader(bool rtcOk, const RtcDateTime* now, bool wifiOk, int rssi);
  void drawWifiIcon(int x, int y, int rssi, bool connected);
  void drawClockIcon(int x, int y, bool rtcConnected);
  
#if TOUCH_SCREEN_ENABLED
  void drawWifiSettingsScreen();
  void drawKeyboard();
  void drawKey(int x, int y, int w, int h, const char* label, bool pressed = false);
  void handleKeyboardTouch(int16_t x, int16_t y);
#endif

public:
#if TOUCH_SCREEN_ENABLED
  bool shouldReconnectWifi = false;
#endif

  UIManager(Adafruit_ILI9341& display, const DisplayConfig& config);

  void begin();

  void setMode(ScreenMode mode);
  ScreenMode getMode() const { return currentMode; }

  void bootScreen(const char* status, const char* ssid);
  void wifiProgress(int attempt, int max);
  void wifiResult(bool ok, const char* ip, int rssi);

  void render(bool rtcOk, const RtcDateTime* now, EmployeeStore& store,
              unsigned long currentMs, bool wifiOk, int rssi);

  void renderNormal(bool rtcOk, const RtcDateTime* now, EmployeeStore& store,
                    unsigned long currentMs, bool wifiOk, int rssi);
                    
  void renderDebug(bool rtcOk, bool wifiOk, int rssi,
                   const String& ssid, const String& ip);

#if TOUCH_SCREEN_ENABLED
  void handleTouch(int16_t x, int16_t y, EmployeeStore& store);
  
  String getNewSsid() const { return wifiInputSsid; }
  String getNewPassword() const { return wifiInputPass; }
  void clearReconnectFlag() { shouldReconnectWifi = false; }
  
  void triggerScan();
  void updateScanResults();
#endif
};

#endif
