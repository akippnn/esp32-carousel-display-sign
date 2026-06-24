#include "rtc.h"

RtcManager::RtcManager(uint8_t datPin, uint8_t clkPin, uint8_t rstPin)
  : wire(datPin, clkPin, rstPin), rtc(wire) {}

bool RtcManager::begin() {
  rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  bool valid = rtc.IsDateTimeValid();

  if (!valid) {
    rtc.SetDateTime(compiled);
    delay(10);
    valid = rtc.IsDateTimeValid();
  }

  if (rtc.GetIsWriteProtected()) {
    rtc.SetIsWriteProtected(false);
  }

  if (!rtc.GetIsRunning()) {
    rtc.SetIsRunning(true);
  }

  return valid;
}

bool RtcManager::isValid() {
  return rtc.IsDateTimeValid();
}

RtcDateTime RtcManager::now() {
  return rtc.GetDateTime();
}
