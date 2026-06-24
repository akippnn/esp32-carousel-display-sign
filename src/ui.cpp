#include "ui.h"
#include <WiFi.h>
#include <Preferences.h>

// ── UIMarquee ──────────────────────────────────────────────────────────────

UIMarquee::UIMarquee(const DisplayConfig& config) : cfg(config) {}

unsigned long UIMarquee::computeFwdMs(int overflow, int maxWidth) const {
  unsigned long ms = (unsigned long)overflow * cfg.marqueeFwd / maxWidth;
  if (ms < cfg.minFwdMs) ms = cfg.minFwdMs;
  return ms;
}

int UIMarquee::position(int textWidth, int maxWidth, unsigned long nowMs, unsigned long maxFwdMs) const {
  if (textWidth <= maxWidth) return 0;

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

  return -(int)(progress * overflow);
}

// ── UIManager ──────────────────────────────────────────────────────────────

// Premium RGB565 Color Palette
static constexpr uint16_t COLOR_BG = 0x0821;             // Dark Indigo/Black (RGB 8, 4, 8)
static constexpr uint16_t COLOR_HEADER = 0x00E3;         // Deep Slate Blue (RGB 0, 28, 30)
static constexpr uint16_t COLOR_TEXT = 0xFFFF;           // White
static constexpr uint16_t COLOR_TEXT_MUTED = 0xBDD7;     // Soft light blue/gray
static constexpr uint16_t COLOR_ACCENT = 0x07FF;         // Teal/Cyan
static constexpr uint16_t COLOR_CARD_BG = 0x10A2;        // Unchecked card background (dark slate)
static constexpr uint16_t COLOR_CARD_BORDER = 0x2124;    // Unchecked card border
static constexpr uint16_t COLOR_CHECKED_IN_BG = 0x0387;  // Checked-in card bg (emerald green/blue)
static constexpr uint16_t COLOR_CHECKED_IN_BORDER = 0x26E4; // Checked-in card border
static constexpr uint16_t COLOR_WIFI_OK = 0x07E0;        // Green
static constexpr uint16_t COLOR_WIFI_ERR = 0xF800;       // Red

UIManager::UIManager(Adafruit_ILI9341& display, const DisplayConfig& config)
  : tft(display), cfg(config), marquee(config) {
    // Initialize text offsets cache with a marker value
    for (int i = 0; i < 12; i++) {
      lastOffsets[i] = 9999;
    }
  }

void UIManager::begin() {
  tft.fillScreen(COLOR_BG);
  
#if TOUCH_SCREEN_ENABLED
  // Load saved credentials if any, otherwise fall back to compiled defaults
  Preferences prefs;
  prefs.begin("wifi-config", true);
  wifiInputSsid = prefs.getString("ssid", WIFI_SSID);
  wifiInputPass = prefs.getString("password", WIFI_PASSWORD);
  prefs.end();
#endif
}

void UIManager::setMode(ScreenMode mode) {
  if (currentMode != mode) {
    currentMode = mode;
    forceRedraw = true;
  }
}

void UIManager::drawClockIcon(int x, int y, bool rtcConnected) {
  tft.drawCircle(x + 8, y + 8, 8, rtcConnected ? COLOR_ACCENT : COLOR_TEXT_MUTED);
  tft.drawLine(x + 8, y + 8, x + 8, y + 4, rtcConnected ? COLOR_ACCENT : COLOR_TEXT_MUTED);
  tft.drawLine(x + 8, y + 8, x + 11, y + 8, rtcConnected ? COLOR_ACCENT : COLOR_TEXT_MUTED);
  if (!rtcConnected) {
    tft.drawLine(x + 3, y + 3, x + 13, y + 13, COLOR_WIFI_ERR);
    tft.drawLine(x + 13, y + 3, x + 3, y + 13, COLOR_WIFI_ERR);
  }
}

void UIManager::drawWifiIcon(int x, int y, int rssi, bool connected) {
  // Clear icon background area in case of changes
  tft.fillRect(x - 2, y - 2, 16, 16, COLOR_HEADER);
  if (!connected) {
    tft.drawLine(x, y + 12, x + 12, y, COLOR_WIFI_ERR);
    tft.drawLine(x, y, x + 12, y + 12, COLOR_WIFI_ERR);
    return;
  }
  uint16_t col = COLOR_WIFI_OK;
  tft.fillRect(x,     y + 9,  2, 3, col);
  tft.fillRect(x + 3, y + 6,  2, 6, rssi >= -80 ? col : COLOR_TEXT_MUTED);
  tft.fillRect(x + 6, y + 3,  2, 9, rssi >= -70 ? col : COLOR_TEXT_MUTED);
  tft.fillRect(x + 9, y,      2, 12, rssi >= -60 ? col : COLOR_TEXT_MUTED);
}

void UIManager::drawHeader(bool rtcOk, const RtcDateTime* now, bool wifiOk, int rssi) {
  // Only redraw static background and divider if fully redrawing
  if (forceRedraw) {
    tft.fillRect(0, 0, cfg.width, 30, COLOR_HEADER);
    tft.drawFastHLine(0, 30, cfg.width, COLOR_ACCENT);
    drawClockIcon(10, 7, rtcOk);
    drawWifiIcon(cfg.width - 22, 9, rssi, wifiOk);
  }

  // Draw WiFi status on change or refresh
  static bool lastWifiOk = false;
  static int lastRssi = 0;
  if (wifiOk != lastWifiOk || abs(rssi - lastRssi) > 5 || forceRedraw) {
    drawWifiIcon(cfg.width - 22, 9, rssi, wifiOk);
    lastWifiOk = wifiOk;
    lastRssi = rssi;
  }

  // Draw time and date if second changes
  if (rtcOk && now) {
    if (now->Second() != lastClockTime || forceRedraw) {
      lastClockTime = now->Second();

      tft.fillRect(32, 2, cfg.width - 64, 26, COLOR_HEADER); // clear text areas
      
      tft.setTextSize(1);
      tft.setTextColor(COLOR_TEXT);
      
      char timeStr[9];
      snprintf(timeStr, sizeof(timeStr), "%02u:%02u:%02u", now->Hour(), now->Minute(), now->Second());
      tft.setCursor(34, 11);
      tft.print(timeStr);

      static const char* months[] = {
        "Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec"
      };
      char dateStr[16];
      snprintf(dateStr, sizeof(dateStr), "%s %u, %04u", months[now->Month() - 1], now->Day(), now->Year());
      int dateLen = strlen(dateStr) * 6;
      tft.setCursor(cfg.width - 34 - dateLen, 11);
      tft.print(dateStr);
    }
  } else if (forceRedraw) {
    tft.fillRect(32, 2, cfg.width - 64, 26, COLOR_HEADER);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(34, 11);
    tft.print("RTC Offline");
  }
}

void UIManager::bootScreen(const char* status, const char* ssid) {
  tft.fillScreen(COLOR_BG);
  tft.drawRoundRect(10, 10, cfg.width - 20, cfg.height - 20, 12, COLOR_ACCENT);
  
  tft.setTextSize(2);
  tft.setTextColor(COLOR_ACCENT);
  tft.setCursor(25, 45);
  tft.print("OJT SIGN IN");

  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(25, 95);
  tft.print("Status: ");
  tft.print(status);

  tft.setCursor(25, 125);
  tft.print("SSID:   ");
  tft.print(ssid);
}

void UIManager::wifiProgress(int attempt, int max) {
  int barW = cfg.width - 80;
  int progressW = map(attempt, 0, max, 0, barW);

  tft.fillRect(40, 150, barW, 14, COLOR_CARD_BG);
  tft.drawRect(40, 150, barW, 14, COLOR_CARD_BORDER);
  tft.fillRect(42, 152, progressW, 10, COLOR_ACCENT);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.fillRect(40, 170, barW, 10, COLOR_BG); // clear status line
  tft.setCursor(40, 170);
  tft.printf("Connecting attempt %d/%d...", attempt, max);
}

void UIManager::wifiResult(bool ok, const char* ip, int rssi) {
  tft.fillScreen(COLOR_BG);
  tft.drawRoundRect(10, 10, cfg.width - 20, cfg.height - 20, 12, ok ? COLOR_WIFI_OK : COLOR_WIFI_ERR);

  tft.setTextSize(2);
  tft.setTextColor(ok ? COLOR_WIFI_OK : COLOR_WIFI_ERR);
  tft.setCursor(30, 45);
  tft.print(ok ? "WiFi Connected!" : "WiFi Failed!");

  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(30, 95);
  tft.printf("IP:   %s", ok ? ip : "N/A");

  tft.setCursor(30, 120);
  tft.printf("RSSI: %d dBm", rssi);
}

// Optimized Flicker-Free Sub-Line drawing
void drawCardLineHelper(Adafruit_ILI9341& tft, UIMarquee& marquee, int cardIdx, int lineIdx, const String& text,
                        int textSize, int16_t cardX, int16_t cardY, int16_t cardW, int16_t lineY,
                        uint16_t bgCol, unsigned long currentMs, int* lastOffsets) {
  int maxTextWidth = cardW - 16;
  int charWidth = textSize * 6;
  int lineHeight = textSize * 8;
  int textWidth = text.length() * charWidth;
  int globalLineIdx = cardIdx * 3 + lineIdx;

  if (textWidth <= maxTextWidth) {
    // Static text (Centered)
    if (lastOffsets[globalLineIdx] == 9999) {
      lastOffsets[globalLineIdx] = 0;
      tft.fillRect(cardX + 8, lineY, maxTextWidth, lineHeight, bgCol);
      tft.setTextSize(textSize);
      tft.setTextColor(lineIdx == 0 ? COLOR_TEXT : (lineIdx == 1 ? COLOR_TEXT_MUTED : COLOR_ACCENT));
      int startX = cardX + 8 + (maxTextWidth - textWidth) / 2;
      tft.setCursor(startX, lineY);
      tft.print(text);
    }
  } else {
    // Scrolling marquee
    int overflow = textWidth - maxTextWidth;
    unsigned long maxFwdMs = marquee.computeFwdMs(overflow, maxTextWidth);
    int offset = marquee.position(textWidth, maxTextWidth, currentMs, maxFwdMs);

    if (offset != lastOffsets[globalLineIdx]) {
      lastOffsets[globalLineIdx] = offset;
      
      tft.fillRect(cardX + 8, lineY, maxTextWidth, lineHeight, bgCol);
      tft.setTextSize(textSize);
      tft.setTextColor(lineIdx == 0 ? COLOR_TEXT : (lineIdx == 1 ? COLOR_TEXT_MUTED : COLOR_ACCENT));
      tft.setCursor(cardX + 8 + offset, lineY);
      tft.print(text);

      // Mask the margins inside the card to create a sharp clipper box
      tft.fillRect(cardX + 1, lineY, 7, lineHeight, bgCol);
      tft.fillRect(cardX + cardW - 8, lineY, 7, lineHeight, bgCol);
    }
  }
}

void UIManager::render(bool rtcOk, const RtcDateTime* now, EmployeeStore& store,
                       unsigned long currentMs, bool wifiOk, int rssi) {
#if TOUCH_SCREEN_ENABLED
  if (isScanning && millis() - scanStartTime >= 4000) {
    updateScanResults();
  }

  if (currentMode == SCREEN_WIFI_SETTINGS) {
    drawWifiSettingsScreen();
  } else
#endif
  if (currentMode == SCREEN_DEBUG) {
    if (forceRedraw || (now && now->Second() != lastClockTime)) {
      forceRedraw = false;
      if (now) lastClockTime = now->Second();
#if TOUCH_SCREEN_ENABLED
      String activeSsid = wifiInputSsid;
#else
      String activeSsid = WIFI_SSID;
#endif
      renderDebug(rtcOk, wifiOk, rssi, activeSsid, WiFi.localIP().toString());
    }
  } else {
    renderNormal(rtcOk, now, store, currentMs, wifiOk, rssi);
  }
}

void UIManager::renderNormal(bool rtcOk, const RtcDateTime* now, EmployeeStore& store,
                            unsigned long currentMs, bool wifiOk, int rssi) {
  // 1. Draw Header (clock and icons)
  drawHeader(rtcOk, now, wifiOk, rssi);

  // 2. Collect active employees
  int activeIndices[10];
  int activeCount = 0;
  if (rtcOk && now) {
    uint8_t today = now->Day();
    uint16_t nowMin = now->Hour() * 60 + now->Minute();
    activeCount = store.collectActive(today, nowMin, activeIndices, 4);
  }

  // 3. Detect Layout changes to perform full redraw
  bool layoutChanged = false;
  if (activeCount != lastActiveCount || forceRedraw) {
    layoutChanged = true;
  } else {
    for (int i = 0; i < activeCount; i++) {
      if (activeIndices[i] != lastActiveIndices[i] ||
          store.get(activeIndices[i]).checkedIn != lastCheckedIn[i]) {
        layoutChanged = true;
        break;
      }
    }
  }

  if (layoutChanged) {
    // Clear screen below header
    tft.fillRect(0, 31, cfg.width, cfg.height - 31, COLOR_BG);
    
    // Save state cache
    lastActiveCount = activeCount;
    for (int i = 0; i < activeCount; i++) {
      lastActiveIndices[i] = activeIndices[i];
      lastCheckedIn[i] = store.get(activeIndices[i]).checkedIn;
    }
    forceRedraw = false;

    // Reset offset cache to force full redraw of all text strings
    for (int i = 0; i < 12; i++) {
      lastOffsets[i] = 9999;
    }

    if (activeCount == 0) {
      tft.drawRoundRect(20, 60, cfg.width - 40, 130, 8, COLOR_CARD_BORDER);
      tft.setTextSize(2);
      tft.setTextColor(COLOR_TEXT_MUTED);
      tft.setCursor(65, 100);
      tft.print("No Active Shifts");
      tft.setTextSize(1);
      tft.setCursor(60, 140);
      tft.print("Waiting for scheduled shift hours");
      return;
    }

    // Draw Static Card Frames & Buttons (No text lines)
    zoneCount = 0;
    for (int i = 0; i < activeCount; i++) {
      Employee& emp = store.getMutable(activeIndices[i]);

      int16_t cardX = 0, cardY = 0, cardW = 0, cardH = 0;
      if (activeCount == 1) {
        cardX = 20; cardY = 50; cardW = 280; cardH = 170;
      } else if (activeCount == 2) {
        cardX = 15 + i * 150; cardY = 50; cardW = 140; cardH = 170;
      } else {
        int row = i / 2;
        int col = i % 2;
        cardX = 15 + col * 150; cardY = 50 + row * 85; cardW = 140; cardH = 75;
      }

      // Save touch zones
      zones[i].xMin = cardX;
      zones[i].xMax = cardX + cardW;
      zones[i].yMin = cardY;
      zones[i].yMax = cardY + cardH;
      zones[i].employeeIndex = activeIndices[i];
      zoneCount++;

      uint16_t bgCol = emp.checkedIn ? COLOR_CHECKED_IN_BG : COLOR_CARD_BG;
      uint16_t borderCol = emp.checkedIn ? COLOR_CHECKED_IN_BORDER : COLOR_CARD_BORDER;

      tft.fillRoundRect(cardX, cardY, cardW, cardH, 8, bgCol);
      tft.drawRoundRect(cardX, cardY, cardW, cardH, 8, borderCol);

      // Draw static buttons inside cards if 1 or 2 cards
      if (activeCount == 1) {
        int btnX = cardX + 30;
        int btnY = cardY + 115;
        int btnW = cardW - 60;
        int btnH = 35;
        uint16_t btnCol = emp.checkedIn ? COLOR_CARD_BG : COLOR_ACCENT;
        tft.fillRoundRect(btnX, btnY, btnW, btnH, 6, btnCol);
        tft.drawRoundRect(btnX, btnY, btnW, btnH, 6, COLOR_TEXT);
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT);
        const char* btnText = emp.checkedIn ? "TAP TO CHECK OUT" : "TAP TO CHECK IN";
        int textLen = strlen(btnText) * 6;
        tft.setCursor(btnX + (btnW - textLen) / 2, btnY + 14);
        tft.print(btnText);
      } else if (activeCount == 2) {
        int btnX = cardX + 10;
        int btnY = cardY + 115;
        int btnW = cardW - 20;
        int btnH = 35;
        uint16_t btnCol = emp.checkedIn ? COLOR_CARD_BG : COLOR_ACCENT;
        tft.fillRoundRect(btnX, btnY, btnW, btnH, 6, btnCol);
        tft.drawRoundRect(btnX, btnY, btnW, btnH, 6, COLOR_TEXT);
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT);
        const char* btnText = emp.checkedIn ? "CHECK OUT" : "CHECK IN";
        int textLen = strlen(btnText) * 6;
        tft.setCursor(btnX + (btnW - textLen) / 2, btnY + 14);
        tft.print(btnText);
      } else { // 3 or 4
        int badgeX = cardX + cardW - 38;
        int badgeY = cardY + cardH - 18;
        int badgeW = 30;
        int badgeH = 12;
        uint16_t badgeCol = emp.checkedIn ? COLOR_WIFI_OK : COLOR_CARD_BORDER;
        tft.fillRoundRect(badgeX, badgeY, badgeW, badgeH, 3, badgeCol);
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT);
        const char* badgeText = emp.checkedIn ? "IN" : "OUT";
        int textLen = strlen(badgeText) * 6;
        tft.setCursor(badgeX + (badgeW - textLen) / 2, badgeY + 2);
        tft.print(badgeText);
      }
    }
  }

  // 4. Draw/Update the texts (Only redraws changing or scrolling marquee offsets)
  for (int i = 0; i < activeCount; i++) {
    Employee& emp = store.getMutable(activeIndices[i]);

    int16_t cardX = 0, cardY = 0, cardW = 0, cardH = 0;
    if (activeCount == 1) {
      cardX = 20; cardY = 50; cardW = 280; cardH = 170;
    } else if (activeCount == 2) {
      cardX = 15 + i * 150; cardY = 50; cardW = 140; cardH = 170;
    } else {
      int row = i / 2;
      int col = i % 2;
      cardX = 15 + col * 150; cardY = 50 + row * 85; cardW = 140; cardH = 75;
    }

    uint16_t bgCol = emp.checkedIn ? COLOR_CHECKED_IN_BG : COLOR_CARD_BG;

    if (activeCount == 1) {
      drawCardLineHelper(tft, marquee, i, 0, emp.lines[0], 2, cardX, cardY, cardW, cardY + 25, bgCol, currentMs, lastOffsets);
      drawCardLineHelper(tft, marquee, i, 1, emp.lines[1], 1, cardX, cardY, cardW, cardY + 60, bgCol, currentMs, lastOffsets);
      drawCardLineHelper(tft, marquee, i, 2, emp.lines[2], 1, cardX, cardY, cardW, cardY + 85, bgCol, currentMs, lastOffsets);
    } else if (activeCount == 2) {
      drawCardLineHelper(tft, marquee, i, 0, emp.lines[0], 1, cardX, cardY, cardW, cardY + 20, bgCol, currentMs, lastOffsets);
      drawCardLineHelper(tft, marquee, i, 1, emp.lines[1], 1, cardX, cardY, cardW, cardY + 50, bgCol, currentMs, lastOffsets);
      drawCardLineHelper(tft, marquee, i, 2, emp.lines[2], 1, cardX, cardY, cardW, cardY + 80, bgCol, currentMs, lastOffsets);
    } else {
      drawCardLineHelper(tft, marquee, i, 0, emp.lines[0], 1, cardX, cardY, cardW, cardY + 12, bgCol, currentMs, lastOffsets);
      drawCardLineHelper(tft, marquee, i, 1, emp.lines[1], 1, cardX, cardY, cardW, cardY + 30, bgCol, currentMs, lastOffsets);
      drawCardLineHelper(tft, marquee, i, 2, emp.lines[2], 1, cardX, cardY, cardW, cardY + 48, bgCol, currentMs, lastOffsets);
    }
  }
}

void UIManager::renderDebug(bool rtcOk, bool wifiOk, int rssi,
                           const String& ssid, const String& ip) {
  tft.fillRect(0, 0, cfg.width, 30, COLOR_HEADER);
  tft.drawFastHLine(0, 30, cfg.width, COLOR_WIFI_ERR);
  
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(15, 11);
  tft.print("[WIFI DIAGNOSTICS & SYSTEM STATUS]");

  tft.fillRect(0, 31, cfg.width, cfg.height - 31, COLOR_BG);

  int startY = 55;
  int lineSpacing = 28;

  auto drawDiagItem = [&](const char* label, const char* value, bool ok) {
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(20, startY);
    tft.print(label);

    tft.setTextColor(ok ? COLOR_WIFI_OK : COLOR_WIFI_ERR);
    tft.setCursor(120, startY);
    tft.print(value);
    
    tft.drawFastHLine(15, startY + 12, cfg.width - 30, COLOR_CARD_BORDER);
    startY += lineSpacing;
  };

  drawDiagItem("SSID:", ssid.c_str(), wifiOk);
  drawDiagItem("IP Address:", wifiOk ? ip.c_str() : "DISCONNECTED", wifiOk);
  
  char rssiStr[16];
  snprintf(rssiStr, sizeof(rssiStr), "%d dBm", rssi);
  drawDiagItem("WiFi RSSI:", wifiOk ? rssiStr : "N/A", wifiOk && rssi >= -80);
  
  drawDiagItem("RTC Sync:", rtcOk ? "ACTIVE" : "FAILED", rtcOk);
  drawDiagItem("Touch Controller:", "ONLINE", true);
}

#if TOUCH_SCREEN_ENABLED

// ── WiFi Settings Screen & Keyboard ──

void UIManager::drawKey(int x, int y, int w, int h, const char* label, bool pressed) {
  uint16_t keyBg = pressed ? COLOR_ACCENT : COLOR_CARD_BG;
  uint16_t keyBorder = COLOR_CARD_BORDER;
  tft.fillRoundRect(x, y, w, h, 4, keyBg);
  tft.drawRoundRect(x, y, w, h, 4, keyBorder);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  int labelLen = strlen(label) * 6;
  tft.setCursor(x + (w - labelLen) / 2, y + (h - 8) / 2);
  tft.print(label);
}

void UIManager::drawWifiSettingsScreen() {
  if (!forceRedraw) return;
  forceRedraw = false;

  // Header
  tft.fillRect(0, 0, cfg.width, 30, COLOR_HEADER);
  tft.drawFastHLine(0, 30, cfg.width, COLOR_ACCENT);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.setCursor(10, 11);
  tft.print("WiFi Network Settings");
  
  // Close / Exit button in header
  tft.drawRoundRect(cfg.width - 50, 4, 40, 22, 4, COLOR_ACCENT);
  tft.setCursor(cfg.width - 43, 11);
  tft.print("EXIT");

  // Body background
  tft.fillRect(0, 31, cfg.width, 89, COLOR_BG);

  // Input Fields
  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setCursor(10, 42);
  tft.print("SSID: ");
  tft.fillRect(50, 35, 260, 16, activeInputField == 0 ? COLOR_CHECKED_IN_BG : COLOR_CARD_BG);
  tft.drawRect(50, 35, 260, 16, activeInputField == 0 ? COLOR_ACCENT : COLOR_CARD_BORDER);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(55, 39);
  tft.print(wifiInputSsid);
  if (activeInputField == 0 && (millis() / 500) % 2 == 0) tft.print("|"); // Blinking cursor

  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setCursor(10, 67);
  tft.print("PASS: ");
  tft.fillRect(50, 60, 260, 16, activeInputField == 1 ? COLOR_CHECKED_IN_BG : COLOR_CARD_BG);
  tft.drawRect(50, 60, 260, 16, activeInputField == 1 ? COLOR_ACCENT : COLOR_CARD_BORDER);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(55, 64);
  // Obfuscate password showing dots
  String dots = "";
  for (size_t i = 0; i < wifiInputPass.length(); i++) dots += "*";
  tft.print(dots);
  if (activeInputField == 1 && (millis() / 500) % 2 == 0) tft.print("|"); // Blinking cursor

  // Scanned Networks List (Y = 85 to 115)
  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setTextSize(1);
  tft.setCursor(10, 93);
  tft.print("Scanned:");

  if (isScanning) {
    tft.setCursor(75, 93);
    tft.print("Scanning WiFi networks...");
  } else if (scannedCount == 0) {
    tft.setCursor(75, 93);
    tft.print("No networks found (Tap to scan)");
  } else {
    for (int i = 0; i < scannedCount && i < 3; i++) {
      int btnX = 70 + i * 82;
      tft.fillRoundRect(btnX, 85, 78, 18, 3, COLOR_CARD_BG);
      tft.drawRoundRect(btnX, 85, 78, 18, 3, COLOR_CARD_BORDER);
      tft.setTextColor(COLOR_TEXT);
      tft.setCursor(btnX + 4, 90);
      
      // Truncate SSID if it's too long
      String s = scannedSsids[i];
      if (s.length() > 11) s = s.substring(0, 9) + "..";
      tft.print(s);
    }
  }

  // Draw Keyboard
  drawKeyboard();
}

void UIManager::drawKeyboard() {
  tft.fillRect(0, 120, cfg.width, 120, COLOR_HEADER);
  tft.drawFastHLine(0, 120, cfg.width, COLOR_CARD_BORDER);

  const char* qwertyNormal[] = {
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p",
    "a", "s", "d", "f", "g", "h", "j", "k", "l",
    "z", "x", "c", "v", "b", "n", "m"
  };
  const char* qwertyShift[] = {
    "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P",
    "A", "S", "D", "F", "G", "H", "J", "K", "L",
    "Z", "X", "C", "V", "B", "N", "M"
  };
  const char* qwertySymbols[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
    "-", "/", ":", ";", "(", ")", "$", "&", "@",
    "_", ".", ",", "?", "!", "'", "\""
  };

  const char** keys = keyboardSymbols ? qwertySymbols : (keyboardShift ? qwertyShift : qwertyNormal);

  // Row 0: 10 keys (Y = 124)
  for (int i = 0; i < 10; i++) {
    drawKey(4 + i * 31, 124, 28, 22, keys[i]);
  }

  // Row 1: 9 keys (Y = 152)
  for (int i = 0; i < 9; i++) {
    drawKey(20 + i * 31, 152, 28, 22, keys[10 + i]);
  }

  // Row 2: Shift, 7 keys, Backspace (Y = 180)
  // Shift Key
  drawKey(4, 180, 36, 22, keyboardSymbols ? "#+=" : (keyboardShift ? "CAPS" : "caps"), keyboardShift);
  // 7 Keys (indices 19 to 25)
  for (int i = 0; i < 7; i++) {
    drawKey(45 + i * 31, 180, 28, 22, keys[19 + i]);
  }
  // Backspace
  drawKey(267, 180, 49, 22, "BACK");

  // Row 3: Mode, Space, Clear, Connect (Y = 208)
  drawKey(4, 208, 44, 22, keyboardSymbols ? "abc" : "?123");
  drawKey(53, 208, 140, 22, "SPACE");
  drawKey(198, 208, 44, 22, "CLR");
  drawKey(247, 208, 69, 22, "CONNECT", true);
}

void UIManager::handleKeyboardTouch(int16_t x, int16_t y) {
  const char* qwertyNormal[] = {
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p",
    "a", "s", "d", "f", "g", "h", "j", "k", "l",
    "z", "x", "c", "v", "b", "n", "m"
  };
  const char* qwertyShift[] = {
    "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P",
    "A", "S", "D", "F", "G", "H", "J", "K", "L",
    "Z", "X", "C", "V", "B", "N", "M"
  };
  const char* qwertySymbols[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
    "-", "/", ":", ";", "(", ")", "$", "&", "@",
    "_", ".", ",", "?", "!", "'", "\""
  };

  const char** keys = keyboardSymbols ? qwertySymbols : (keyboardShift ? qwertyShift : qwertyNormal);

  String* activeInput = (activeInputField == 0) ? &wifiInputSsid : &wifiInputPass;

  // Row 0
  if (y >= 124 && y < 146) {
    int idx = (x - 4) / 31;
    if (idx >= 0 && idx < 10) {
      activeInput->concat(keys[idx]);
      forceRedraw = true;
    }
  }
  // Row 1
  else if (y >= 152 && y < 174) {
    int idx = (x - 20) / 31;
    if (idx >= 0 && idx < 9) {
      activeInput->concat(keys[10 + idx]);
      forceRedraw = true;
    }
  }
  // Row 2: Shift (caps), 7 keys, Backspace
  else if (y >= 180 && y < 202) {
    if (x >= 4 && x < 40) {
      keyboardShift = !keyboardShift;
      forceRedraw = true;
    } else if (x >= 45 && x < 262) {
      int idx = (x - 45) / 31;
      if (idx >= 0 && idx < 7) {
        activeInput->concat(keys[19 + idx]);
        forceRedraw = true;
      }
    } else if (x >= 267 && x < 316) {
      // Backspace
      if (activeInput->length() > 0) {
        *activeInput = activeInput->substring(0, activeInput->length() - 1);
        forceRedraw = true;
      }
    }
  }
  // Row 3: Mode, Space, Clear, Connect
  else if (y >= 208 && y < 230) {
    if (x >= 4 && x < 48) {
      keyboardSymbols = !keyboardSymbols;
      forceRedraw = true;
    } else if (x >= 53 && x < 193) {
      // Space
      activeInput->concat(" ");
      forceRedraw = true;
    } else if (x >= 198 && x < 242) {
      // Clear
      *activeInput = "";
      forceRedraw = true;
    } else if (x >= 247 && x < 316) {
      // Connect!
      Preferences prefs;
      prefs.begin("wifi-config", false);
      prefs.putString("ssid", wifiInputSsid);
      prefs.putString("password", wifiInputPass);
      prefs.end();
      
      Serial.println("Saved custom WiFi to Preferences");
      shouldReconnectWifi = true;
      setMode(SCREEN_NORMAL);
    }
  }
}

void UIManager::handleTouch(int16_t x, int16_t y, EmployeeStore& store) {
  // ── Mode Toggle / Close Touch Zones ──
  if (currentMode == SCREEN_NORMAL) {
    // Tap WiFi icon to configure WiFi Settings
    if (x >= 280 && y <= 30) {
      setMode(SCREEN_WIFI_SETTINGS);
      triggerScan();
      return;
    }

    // Tap employee cards to toggle check-in state
    for (int i = 0; i < zoneCount; i++) {
      if (x >= zones[i].xMin && x <= zones[i].xMax &&
          y >= zones[i].yMin && y <= zones[i].yMax) {
        int idx = zones[i].employeeIndex;
        if (idx >= 0 && idx < store.size()) {
          Employee& emp = store.getMutable(idx);
          emp.checkedIn = !emp.checkedIn;
          Serial.printf("UI: Toggled check-in for %s: %s\r\n", emp.lines[0].c_str(), emp.checkedIn ? "IN" : "OUT");
          break;
        }
      }
    }
  } 
  
  else if (currentMode == SCREEN_WIFI_SETTINGS) {
    // Exit button in header
    if (x >= cfg.width - 55 && y <= 30) {
      setMode(SCREEN_NORMAL);
      return;
    }

    // Select Input Fields
    if (y >= 35 && y < 51) {
      activeInputField = 0;
      forceRedraw = true;
    } else if (y >= 60 && y < 76) {
      activeInputField = 1;
      forceRedraw = true;
    }
    
    // Scanned Networks buttons (Y = 85..103)
    else if (y >= 85 && y < 103) {
      for (int i = 0; i < scannedCount && i < 3; i++) {
        int btnX = 70 + i * 82;
        if (x >= btnX && x < btnX + 78) {
          wifiInputSsid = scannedSsids[i];
          activeInputField = 1; // switch focus to password
          forceRedraw = true;
          break;
        }
      }
    }

    // Keyboard touch
    else if (y >= 120) {
      handleKeyboardTouch(x, y);
    }
  }
}

void UIManager::triggerScan() {
  if (isScanning) return;
  Serial.println("WiFi: Starting asynchronous network scan...");
  scannedCount = 0;
  isScanning = true;
  scanStartTime = millis();
  WiFi.scanNetworks(true); // Asynchronous scan
}

void UIManager::updateScanResults() {
  isScanning = false;
  int n = WiFi.scanComplete();
  if (n >= 0) {
    scannedCount = n < 3 ? n : 3;
    for (int i = 0; i < scannedCount; i++) {
      scannedSsids[i] = WiFi.SSID(i);
      Serial.printf("WiFi: Found SSID: %s\r\n", scannedSsids[i].c_str());
    }
  } else {
    scannedCount = 0;
  }
  WiFi.scanDelete();
  forceRedraw = true;
}

#endif
