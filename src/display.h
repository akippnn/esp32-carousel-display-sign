#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"

class MarqueeEngine {
  const DisplayConfig& cfg;

public:
  MarqueeEngine(const DisplayConfig& config);

  unsigned long computeFwdMs(int overflow, int maxWidth) const;
  int position(int textWidth, int maxWidth, unsigned long nowMs, unsigned long maxFwdMs) const;
};

class DisplayRenderer {
  U8G2& gfx;
  const DisplayConfig& cfg;
  MarqueeEngine marquee;

  void drawWifiIcon(int x, int y, int rssi, bool connected) const;
  void drawClockIcon(int x, int y, bool rtcConnected) const;

public:
  DisplayRenderer(U8G2& display, const DisplayConfig& config);

  void begin();

  void bootScreen(const char* status, const char* ssid);
  void wifiProgress(int attempt, int max);
  void wifiResult(bool ok, const char* ip, int rssi);

  void renderNormal(bool rtcOk, const RtcDateTime* now, const String lines[3],
                    unsigned long currentMs, bool wifiOk, int rssi,
                    const String& ssid);
  void renderDebug(bool rtcOk, bool wifiOk, int rssi,
                   const String& ssid, const String& ip);
};

#endif
