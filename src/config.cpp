#include "config.h"

const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;

WebServer server(80);

bool debugMode = false;

U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/ 23, /* data=*/ 22, /* cs=*/ 21, /* reset=*/ 4);

ThreeWire myWire(18, 19, 5);
RtcDS1302<ThreeWire> Rtc(myWire);

const char* supabaseUrl = SUPABASE_URL;
const char* supabaseKey = SUPABASE_KEY;

String employeeData[maxEmployees][3];
int employeeCount = 0;
unsigned long lastSupabaseFetch = 0;

uint8_t employeeScheduleDay[maxEmployees] = {0};
uint16_t employeeStartMinutes[maxEmployees] = {0};
uint16_t employeeEndMinutes[maxEmployees] = {0};
