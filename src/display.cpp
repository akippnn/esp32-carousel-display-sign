#include "display.h"

DisplayDriver::DisplayDriver(const DisplayConfig& config, SPIClass* spi)
  : tft(spi, config.pinTftDc, config.pinTftCs, config.pinTftRst), cfg(config) {}

void DisplayDriver::begin() {
  // Perform a manual hardware reset to guarantee the driver chip boots from a clean state.
  pinMode(cfg.pinTftRst, OUTPUT);
  digitalWrite(cfg.pinTftRst, LOW);
  delay(100);
  digitalWrite(cfg.pinTftRst, HIGH);
  delay(150);

  tft.begin(16000000); // Set clock speed to 16MHz for solid signal integrity over GPIO matrix
  tft.setRotation(1); // Landscape mode (320x240)
  tft.fillScreen(ILI9341_BLACK);
}
