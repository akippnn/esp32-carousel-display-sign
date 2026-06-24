#ifndef APP_H
#define APP_H

#include "config.h"
#include "display.h"
#include "network.h"
#include "rtc.h"
#include "employee.h"
#include "supabase.h"

class WebServer;

class App {
  DisplayConfig cfg;
  U8G2_ST7920_128X64_F_SW_SPI gfx;
  RtcManager rtc;
  WifiManager wifi;
  DisplayRenderer renderer;
  EmployeeStore employees;
  SupabaseClient supabase;
  WebServer* server;

  bool debugMode = false;
  unsigned long lastRefresh = 0;

  void startServer();
  void handleStatus();
  void handleToggleDebug();
  void handleSerial();
  void runDisplay(unsigned long now);
  void fetchEmployees(unsigned long now);

public:
  App();
  void setup();
  void loop();
};

#endif
