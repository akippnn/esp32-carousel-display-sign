#ifndef RTC_H
#define RTC_H

#include "config.h"

class RtcManager {
  ThreeWire wire;
  RtcDS1302<ThreeWire> rtc;

public:
  RtcManager(uint8_t datPin, uint8_t clkPin, uint8_t rstPin);

  bool begin();
  bool isValid();
  RtcDateTime now();
};

#endif
