#include "config.h"
#include "display.h"

void drawWifiIcon(int x, int y, int rssi, bool connected) {
  if (!connected) {
    u8g2.drawLine(x, y + 1, x + 4, y + 5);
    u8g2.drawLine(x + 4, y + 1, x, y + 5);
    return;
  }
  // Bar 1
  u8g2.drawLine(x, y + 4, x, y + 5);
  // Bar 2 (>= -80 dBm)
  if (rssi >= -80) {
    u8g2.drawLine(x + 2, y + 2, x + 2, y + 5);
  }
  // Bar 3 (>= -70 dBm)
  if (rssi >= -70) {
    u8g2.drawLine(x + 4, y, x + 4, y + 5);
  }
}

void drawClockIcon(int x, int y, bool rtcConnected) {
  if (rtcConnected) {
    u8g2.setDrawColor(1);
    u8g2.drawCircle(x + 3, y + 3, 3, U8G2_DRAW_ALL);
    u8g2.drawPixel(x + 3, y + 3);       // center
    u8g2.drawPixel(x + 3, y + 2);       // minute hand (up)
    u8g2.drawPixel(x + 2, y + 3);       // hour hand (left)
  } else {
    u8g2.setDrawColor(1);
    u8g2.drawCircle(x + 3, y + 3, 3, U8G2_DRAW_ALL);
    u8g2.drawPixel(x + 3, y + 3);
    u8g2.drawPixel(x + 3, y + 2);
    u8g2.drawPixel(x + 2, y + 3);

    u8g2.setDrawColor(0);
    u8g2.drawBox(x + 4, y + 4, 3, 3);
    u8g2.drawPixel(x + 3, y + 4);
    u8g2.drawPixel(x + 4, y + 3);
    u8g2.drawPixel(x + 3, y + 5);
    u8g2.drawPixel(x + 5, y + 3);
    u8g2.drawPixel(x + 3, y + 6);
    u8g2.drawPixel(x + 6, y + 3);

    u8g2.setDrawColor(1);
    u8g2.drawPixel(x + 4, y + 4);
    u8g2.drawPixel(x + 6, y + 4);
    u8g2.drawPixel(x + 5, y + 5);
    u8g2.drawPixel(x + 4, y + 6);
    u8g2.drawPixel(x + 6, y + 6);
  }
}

// Smooth pixel marquee — all lines share maxFwdMs so they sync.
//   [forward scroll]   each line at own speed — fast lines wait at end
//   [hold 3s]          all lines at end
//   [scroll back]      all share same phase — longer text moves faster, arrive together
//   [hold 3s]          all lines at start
int marqueeX(int textWidth, int maxWidth, unsigned long nowMs, int lineIndex, unsigned long maxFwdMs) {
  (void)lineIndex;
  const int baseX = 6;
  if (textWidth <= maxWidth) return baseX;

  int overflow = textWidth - maxWidth;

  unsigned long fwdMs = (unsigned long)overflow * 400 / maxWidth;
  if (fwdMs < 300) fwdMs = 300;

  unsigned long holdEndMs = 3000;
  unsigned long holdStartMs = 3000;
  unsigned long cycleMs = maxFwdMs + holdEndMs + maxFwdMs + holdStartMs;

  unsigned long t = nowMs % cycleMs;

  float progress;
  if (t < maxFwdMs) {
    progress = (float)t / fwdMs;
    if (progress > 1.0f) progress = 1.0f;
  } else if (t < maxFwdMs + holdEndMs) {
    progress = 1.0f;
  } else if (t < maxFwdMs + holdEndMs + maxFwdMs) {
    progress = 1.0f - (float)(t - maxFwdMs - holdEndMs) / maxFwdMs;
  } else {
    progress = 0.0f;
  }

  return baseX - (int)(progress * overflow);
}
