#ifndef NETWORK_H
#define NETWORK_H

#include "config.h"

class DisplayRenderer;

class WifiManager {
  const char* ssid;
  const char* password;
  uint16_t retryMs;
  uint16_t attemptMs;
  uint8_t maxAttempts;
  unsigned long lastRetry = 0;

public:
  WifiManager(const char* ssid, const char* pass, uint16_t retry, uint16_t attempt, uint8_t max);

  bool connect(DisplayRenderer& display);
  bool isConnected() const;
  int rssi() const;
  String ip() const;
  void poll();
};

#endif
