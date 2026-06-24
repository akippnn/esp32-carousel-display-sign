#ifndef CONFIG_H
#define CONFIG_H

#include "env_credentials.h"
#include <Arduino.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

#ifndef WIFI_SSID
  #error "WIFI_SSID not defined. Add WIFI_SSID=your_wifi to .env"
#endif
#if !DATABASE_PROVIDER_FIREBASE
#ifndef SUPABASE_URL
  #error "SUPABASE_URL not defined. Ensure .env exists and load_env.py is configured in platformio.ini."
#endif
#ifndef SUPABASE_KEY
  #error "SUPABASE_KEY not defined. Ensure .env exists and load_env.py is configured in platformio.ini."
#endif
#endif

static constexpr int MAX_EMPLOYEES = 10;
static constexpr unsigned long SUPABASE_FETCH_INTERVAL = 30000;
static constexpr unsigned long SERIAL_BAUD = 115200;

struct DisplayConfig {
  uint16_t width = 320;
  uint16_t height = 240;

  // SPI Bus
  uint8_t pinClk = 5;
  uint8_t pinMosi = 17;
  uint8_t pinMiso = 26; // T_DO (MISO)

  // TFT display
  uint8_t pinTftCs = 15;
  uint8_t pinTftDc = 4;
  uint8_t pinTftRst = 2;

  // Touch controller
  uint8_t pinTouchCs = 27;
  uint8_t pinTouchIrq = 35;

  // RTC
  uint8_t rtcClk = 25;
  uint8_t rtcRst = 32;
  uint8_t rtcData = 33;

  uint8_t marginLeft = 10;
  uint16_t contentMaxWidth;

  uint16_t refreshMs = 40;
  uint16_t rotationMs = 5000;

  uint16_t wifiRetryMs = 10000;
  uint16_t wifiAttemptMs = 500;
  uint8_t wifiMaxAttempts = 60;

  uint16_t marqueeFwd = 400;
  uint16_t marqueeRet = 120;
  uint16_t holdEndMs = 3000;
  uint16_t holdStartMs = 3000;
  uint16_t minFwdMs = 300;
  uint16_t minRetMs = 80;

  DisplayConfig() {
    contentMaxWidth = width - marginLeft * 2;
  }
};

struct SupabaseFieldMapping {
  const char* firstName = SUPABASE_FIELD_FIRST_NAME;
  const char* lastName  = SUPABASE_FIELD_LAST_NAME;
  const char* position  = SUPABASE_FIELD_POSITION;
  const char* start     = SUPABASE_FIELD_START;
  const char* end       = SUPABASE_FIELD_END;
};

#endif
