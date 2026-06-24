#ifndef TOUCH_H
#define TOUCH_H

#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "config.h"

struct TouchPoint {
  int16_t x = -1;
  int16_t y = -1;
  bool touched = false;
};

class TouchManager {
  XPT2046_Touchscreen ts;
  const DisplayConfig& cfg;
  TouchPoint lastPoint;
  unsigned long lastPollTime = 0;
  static constexpr unsigned long POLL_INTERVAL_MS = 20;

public:
  TouchManager(const DisplayConfig& config);

  void begin(SPIClass& spi);
  
  // Non-blocking poll. Returns true if a new touch state change occurred.
  bool poll();

  bool isTouched() const { return lastPoint.touched; }
  TouchPoint getTouchPoint() const { return lastPoint; }
};

#endif
