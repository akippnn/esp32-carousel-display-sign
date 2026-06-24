#include "display.h"

DisplayDriver::DisplayDriver(const DisplayConfig& config, SPIClass* spi)
  : tft(spi, config.pinTftDc, config.pinTftCs, config.pinTftRst), cfg(config) {}

void DisplayDriver::begin() {
  tft.begin();
  tft.setRotation(1); // Landscape mode (320x240)
  tft.fillScreen(ILI9341_BLACK);
}
