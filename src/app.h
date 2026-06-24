#ifndef APP_H
#define APP_H

#include "config.h"
#include "display.h"
#include "touch.h"
#include "ui.h"
#include "network.h"
#include "rtc.h"
#include "employee.h"
#include "supabase.h"

class WebServer;

class App {
  DisplayConfig cfg;
  SPIClass spiBus;
  DisplayDriver display;
  TouchManager touch;
  UIManager ui;
  RtcManager rtc;
  WifiManager wifi;
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
