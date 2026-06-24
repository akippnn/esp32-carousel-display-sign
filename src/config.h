#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

#ifndef WIFI_SSID
  #error "WIFI_SSID not defined. Add WIFI_SSID=your_wifi to .env"
#endif
#ifndef SUPABASE_URL
  #error "SUPABASE_URL not defined. Ensure .env exists and load_env.py is configured in platformio.ini."
#endif
#ifndef SUPABASE_KEY
  #error "SUPABASE_KEY not defined. Ensure .env exists and load_env.py is configured in platformio.ini."
#endif

static constexpr int MAX_EMPLOYEES = 10;
static constexpr unsigned long SUPABASE_FETCH_INTERVAL = 30000;
static constexpr unsigned long SERIAL_BAUD = 115200;

struct DisplayConfig {
  uint8_t width = 128;
  uint8_t height = 64;

  uint8_t pinClk = 23;
  uint8_t pinData = 22;
  uint8_t pinCs = 21;
  uint8_t pinReset = 4;

  uint8_t rtcData = 18;
  uint8_t rtcClk = 19;
  uint8_t rtcRst = 5;

  uint8_t marginLeft = 6;
  uint8_t contentMaxWidth;
  uint8_t dividerY = 17;
  uint8_t lineY[3] = {32, 46, 57};

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

  const uint8_t* fontSmall = u8g2_font_6x10_tf;
  const uint8_t* fontLarge = u8g2_font_helvB10_tf;

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
