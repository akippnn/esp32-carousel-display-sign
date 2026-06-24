#include "display.h"

// ── MarqueeEngine ──────────────────────────────────────────────────────────

MarqueeEngine::MarqueeEngine(const DisplayConfig& config) : cfg(config) {}

unsigned long MarqueeEngine::computeFwdMs(int overflow, int maxWidth) const {
  unsigned long ms = (unsigned long)overflow * cfg.marqueeFwd / maxWidth;
  if (ms < cfg.minFwdMs) ms = cfg.minFwdMs;
  return ms;
}

int MarqueeEngine::position(int textWidth, int maxWidth, unsigned long nowMs, unsigned long maxFwdMs) const {
  if (textWidth <= maxWidth) return cfg.marginLeft;

  int overflow = textWidth - maxWidth;
  unsigned long fwdMs = computeFwdMs(overflow, maxWidth);
  unsigned long cycleMs = maxFwdMs + cfg.holdEndMs + maxFwdMs + cfg.holdStartMs;
  unsigned long t = nowMs % cycleMs;

  float progress;
  if (t < maxFwdMs) {
    progress = (float)t / fwdMs;
    if (progress > 1.0f) progress = 1.0f;
  } else if (t < maxFwdMs + cfg.holdEndMs) {
    progress = 1.0f;
  } else if (t < maxFwdMs + cfg.holdEndMs + maxFwdMs) {
    progress = 1.0f - (float)(t - maxFwdMs - cfg.holdEndMs) / maxFwdMs;
  } else {
    progress = 0.0f;
  }

  return cfg.marginLeft - (int)(progress * overflow);
}

// ── DisplayRenderer ────────────────────────────────────────────────────────

DisplayRenderer::DisplayRenderer(U8G2& display, const DisplayConfig& config)
  : gfx(display), cfg(config), marquee(config) {}

void DisplayRenderer::begin() {
  gfx.begin();
}

void DisplayRenderer::drawWifiIcon(int x, int y, int rssi, bool connected) const {
  if (!connected) {
    gfx.drawLine(x, y + 1, x + 4, y + 5);
    gfx.drawLine(x + 4, y + 1, x, y + 5);
    return;
  }
  gfx.drawLine(x, y + 4, x, y + 5);
  if (rssi >= -80) gfx.drawLine(x + 2, y + 2, x + 2, y + 5);
  if (rssi >= -70) gfx.drawLine(x + 4, y, x + 4, y + 5);
}

void DisplayRenderer::drawClockIcon(int x, int y, bool rtcConnected) const {
  auto drawOK = [&] {
    gfx.setDrawColor(1);
    gfx.drawCircle(x + 3, y + 3, 3, U8G2_DRAW_ALL);
    gfx.drawPixel(x + 3, y + 3);
    gfx.drawPixel(x + 3, y + 2);
    gfx.drawPixel(x + 2, y + 3);
  };

  if (rtcConnected) {
    drawOK();
  } else {
    drawOK();
    gfx.setDrawColor(0);
    gfx.drawBox(x + 4, y + 4, 3, 3);
    gfx.drawPixel(x + 3, y + 4);
    gfx.drawPixel(x + 4, y + 3);
    gfx.drawPixel(x + 3, y + 5);
    gfx.drawPixel(x + 5, y + 3);
    gfx.drawPixel(x + 3, y + 6);
    gfx.drawPixel(x + 6, y + 3);

    gfx.setDrawColor(1);
    gfx.drawPixel(x + 4, y + 4);
    gfx.drawPixel(x + 6, y + 4);
    gfx.drawPixel(x + 5, y + 5);
    gfx.drawPixel(x + 4, y + 6);
    gfx.drawPixel(x + 6, y + 6);
  }
}

void DisplayRenderer::bootScreen(const char* status, const char* ssid) {
  gfx.clearBuffer();
  gfx.drawFrame(0, 0, cfg.width, cfg.height);
  gfx.setFont(cfg.fontSmall);
  gfx.drawStr(10, 16, status);
  gfx.drawStr(10, 30, ssid);
  gfx.sendBuffer();
}

void DisplayRenderer::wifiProgress(int attempt, int max) {
  const char* spinner = "|/-\\";
  char buf[16];
  snprintf(buf, sizeof(buf), "%c  %d/%ds", spinner[attempt % 4], attempt, attempt * cfg.wifiAttemptMs / 1000);

  gfx.setDrawColor(0);
  gfx.drawBox(10, 42, 108, 12);
  gfx.setDrawColor(1);
  gfx.drawStr(10, 52, buf);
  gfx.sendBuffer();
}

void DisplayRenderer::wifiResult(bool ok, const char* ip, int rssi) {
  gfx.clearBuffer();
  gfx.drawFrame(0, 0, cfg.width, cfg.height);
  if (ok) {
    gfx.setFont(cfg.fontSmall);
    gfx.drawStr(10, 20, "WiFi Connected!");
    gfx.setCursor(10, 34);
    gfx.print(ip);
    char buf[32];
    snprintf(buf, sizeof(buf), "RSSI: %d dBm", rssi);
    gfx.drawStr(10, 48, buf);
  } else {
    gfx.setFont(cfg.fontLarge);
    gfx.drawStr(6, 28, "WiFi Offline");
    gfx.setFont(cfg.fontSmall);
    gfx.drawStr(10, 44, "Will keep retrying...");
  }
  gfx.sendBuffer();
}

void DisplayRenderer::renderNormal(bool rtcOk, const RtcDateTime* now, const String lines[3],
                                    unsigned long currentMs, bool wifiOk, int rssi,
                                    const String& ssid) {
  (void)ssid;
  gfx.clearBuffer();
  gfx.drawFrame(0, 0, cfg.width, cfg.height);

  drawClockIcon(cfg.marginLeft, 6, rtcOk);
  drawWifiIcon(cfg.width - cfg.marginLeft - 6, 6, rssi, wifiOk);

  if (rtcOk && now) {
    gfx.setFont(cfg.fontSmall);

    char hours[3], minutes[3];
    snprintf(hours, sizeof(hours), "%02u", now->Hour());
    snprintf(minutes, sizeof(minutes), "%02u", now->Minute());

    gfx.drawStr(16, 13, hours);
    if (now->Second() % 2 == 0) {
      gfx.drawStr(28, 12, ":");
    }
    gfx.drawStr(34, 13, minutes);

    static const char* months[] = {
      "Jan","Feb","Mar","Apr","May","Jun",
      "Jul","Aug","Sep","Oct","Nov","Dec"
    };
    char dateStr[8];
    snprintf(dateStr, sizeof(dateStr), "%s %u", months[now->Month() - 1], now->Day());
    gfx.drawStr(52, 13, dateStr);
  }

  gfx.drawHLine(cfg.marginLeft, cfg.dividerY, cfg.contentMaxWidth);

  unsigned long maxFwdMs = 0;
  int textWidths[3] = {0, 0, 0};

  for (int i = 0; i < 3; i++) {
    if (lines[i].length() == 0) continue;
    gfx.setFont(i == 0 ? cfg.fontLarge : cfg.fontSmall);
    textWidths[i] = gfx.getStrWidth(lines[i].c_str());
    int overflow = textWidths[i] - cfg.contentMaxWidth;
    if (overflow > 0) {
      unsigned long fwd = marquee.computeFwdMs(overflow, cfg.contentMaxWidth);
      if (fwd > maxFwdMs) maxFwdMs = fwd;
    }
  }

  for (int i = 0; i < 3; i++) {
    if (lines[i].length() == 0) continue;
    gfx.setFont(i == 0 ? cfg.fontLarge : cfg.fontSmall);
    int x = marquee.position(textWidths[i], cfg.contentMaxWidth, currentMs, maxFwdMs);
    gfx.drawStr(x, cfg.lineY[i], lines[i].c_str());
  }

  gfx.sendBuffer();
}

void DisplayRenderer::renderDebug(bool rtcOk, bool wifiOk, int rssi,
                                   const String& ssid, const String& ip) {
  gfx.clearBuffer();
  gfx.drawFrame(0, 0, cfg.width, cfg.height);
  gfx.setFont(cfg.fontSmall);
  gfx.drawStr(6, 12, "[WiFi Diagnostic]");
  gfx.drawHLine(4, 15, 120);

  gfx.setCursor(6, 28);
  gfx.print("SSID: ");
  gfx.print(ssid);

  gfx.setCursor(6, 40);
  gfx.print("IP  : ");
  gfx.print(wifiOk ? ip : "Disconnected");

  gfx.setCursor(6, 52);
  gfx.print("RSSI: ");
  if (wifiOk) {
    gfx.print(rssi);
    gfx.print(" dBm | RTC: ");
    gfx.print(rtcOk ? "OK" : "ERR");
  } else {
    gfx.print("N/A");
  }

  gfx.sendBuffer();
}
