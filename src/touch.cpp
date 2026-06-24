#include "touch.h"

TouchManager::TouchManager(const DisplayConfig& config)
  : ts(config.pinTouchCs, config.pinTouchIrq), cfg(config) {}

void TouchManager::begin(SPIClass& spi) {
  pinMode(cfg.pinTouchIrq, INPUT_PULLUP);
  ts.begin(spi);
  ts.setRotation(1); // Set rotation to match the screen landscape mode
}

bool TouchManager::poll() {
  unsigned long now = millis();
  if (now - lastPollTime < POLL_INTERVAL_MS) {
    return false;
  }
  lastPollTime = now;

  // The T_IRQ pin goes LOW when the screen is touched.
  // Polling T_IRQ avoids querying the touchscreen chip over SPI unnecessarily.
  bool touched = (digitalRead(cfg.pinTouchIrq) == LOW);

  if (touched) {
    if (ts.touched()) {
      TS_Point p = ts.getPoint();
      
      // Calibration mapping: map raw resistive touch values to display coordinates.
      // Raw X/Y ranges are typically around 200 to 3800.
      // In landscape (rotation 1), raw coordinates might need axes swapping or inversion:
      // X = map(p.x, 200, 3800, 0, width);
      // Y = map(p.y, 200, 3800, 0, height);
      // Let's perform a robust mapping and constrain values.
      int16_t mappedX = map(p.x, 200, 3800, 0, cfg.width);
      int16_t mappedY = map(p.y, 200, 3800, 0, cfg.height);

      mappedX = constrain(mappedX, 0, cfg.width - 1);
      mappedY = constrain(mappedY, 0, cfg.height - 1);

      bool changed = (!lastPoint.touched || lastPoint.x != mappedX || lastPoint.y != mappedY);
      
      if (changed) {
        Serial.printf("Touch detected: Raw(%d, %d) -> Mapped(%d, %d)\r\n", p.x, p.y, mappedX, mappedY);
      }

      lastPoint.x = mappedX;
      lastPoint.y = mappedY;
      lastPoint.touched = true;
      return changed;
    }
  }

  // Touch released
  if (lastPoint.touched) {
    lastPoint.touched = false;
    lastPoint.x = -1;
    lastPoint.y = -1;
    Serial.println("Touch released");
    return true;
  }

  return false;
}
