#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "config.h"

class DisplayDriver {
  Adafruit_ILI9341 tft;
  const DisplayConfig& cfg;

public:
  DisplayDriver(const DisplayConfig& config, SPIClass* spi);

  void begin();
  
  Adafruit_ILI9341& getTft() { return tft; }
  const DisplayConfig& getConfig() const { return cfg; }
};

#endif
