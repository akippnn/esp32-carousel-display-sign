#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <WiFi.h>
#include <WebServer.h>

#ifndef WIFI_SSID
  #error "WIFI_SSID not defined. Add WIFI_SSID=your_wifi to .env"
#endif
#ifndef SUPABASE_URL
  #error "SUPABASE_URL not defined. Ensure .env exists and load_env.py is configured in platformio.ini."
#endif
#ifndef SUPABASE_KEY
  #error "SUPABASE_KEY not defined. Ensure .env exists and load_env.py is configured in platformio.ini."
#endif

constexpr int maxEmployees = 10;
constexpr unsigned long supabaseFetchInterval = 30000;

extern const char* ssid;
extern const char* password;

extern WebServer server;
extern bool debugMode;

extern U8G2_ST7920_128X64_F_SW_SPI u8g2;

extern ThreeWire myWire;
extern RtcDS1302<ThreeWire> Rtc;

extern const char* supabaseUrl;
extern const char* supabaseKey;

extern String employeeData[maxEmployees][3];
extern int employeeCount;
extern unsigned long lastSupabaseFetch;

extern uint8_t employeeScheduleDay[maxEmployees];
extern uint16_t employeeStartMinutes[maxEmployees];
extern uint16_t employeeEndMinutes[maxEmployees];

#endif
