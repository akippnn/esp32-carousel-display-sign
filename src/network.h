#ifndef NETWORK_H
#define NETWORK_H

#include "config.h"

class UIManager;

class WifiManager {
  String ssid;
  String password;
  uint16_t retryMs;
  uint16_t attemptMs;
  uint8_t maxAttempts;
  unsigned long lastRetry = 0;

public:
  WifiManager(const String& ssid, const String& pass, uint16_t retry, uint16_t attempt, uint8_t max);

  bool connect(UIManager& ui);
  bool isConnected() const;
  int rssi() const;
  String ip() const;
  void poll();
  void updateCredentials(const String& newSsid, const String& newPass);
};

#endif
