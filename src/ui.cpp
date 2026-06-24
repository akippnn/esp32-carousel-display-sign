#include "ui.h"

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
  : tft(display), cfg(config), marquee(config) {}

void UIManager::begin() {
  tft.fillScreen(COLOR_BG);
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
  // Draw header background bar
  tft.fillRect(0, 0, cfg.width, 30, COLOR_HEADER);
  tft.drawFastHLine(0, 30, cfg.width, COLOR_ACCENT);

  // Draw RTC and WiFi status icons
  drawClockIcon(10, 7, rtcOk);
  drawWifiIcon(cfg.width - 22, 9, rssi, wifiOk);

  // Render Time and Date
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT);
  
  if (rtcOk && now) {
    char timeStr[9];
    snprintf(timeStr, sizeof(timeStr), "%02u:%02u:%02u", now->Hour(), now->Minute(), now->Second());
    tft.setCursor(34, 11);
    tft.setTextSize(1);
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
  } else {
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
  tft.print("Target SSID: ");
  tft.print(ssid);

  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setCursor(25, 185);
  tft.print("Initializing hardware modules...");
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
  tft.printf("SSID: %s", WIFI_SSID);

  tft.setCursor(30, 120);
  tft.printf("IP:   %s", ok ? ip : "N/A");

  tft.setCursor(30, 145);
  tft.printf("RSSI: %d dBm", rssi);

  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setCursor(30, 185);
  tft.print("Starting main state loops...");
}

void UIManager::renderNormal(bool rtcOk, const RtcDateTime* now, EmployeeStore& store,
                            unsigned long currentMs, bool wifiOk, int rssi) {
  // 1. Draw Header
  drawHeader(rtcOk, now, wifiOk, rssi);

  // 2. Collect active employees
  int activeIndices[10];
  int activeCount = 0;
  if (rtcOk && now) {
    uint8_t today = now->Day();
    uint16_t nowMin = now->Hour() * 60 + now->Minute();
    activeCount = store.collectActive(today, nowMin, activeIndices, 4); // Limit to 4 cards max on screen
  }

  // 3. Clear the area below header if needed
  // We will do clean over-writing instead of clearScreen to prevent screen flicker.
  // When activeCount goes to 0 or changes, we erase card locations.
  // To keep it simple, we draw/erase depending on active count.
  zoneCount = 0;

  if (activeCount == 0) {
    tft.fillRect(0, 31, cfg.width, cfg.height - 31, COLOR_BG);
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

  // 4. Render Layouts dynamically
  for (int i = 0; i < activeCount; i++) {
    int empIdx = activeIndices[i];
    Employee& emp = store.getMutable(empIdx);

    int16_t cardX = 0, cardY = 0, cardW = 0, cardH = 0;
    
    if (activeCount == 1) {
      cardX = 20; cardY = 50; cardW = 280; cardH = 170;
    } else if (activeCount == 2) {
      cardX = 15 + i * 150; cardY = 50; cardW = 140; cardH = 170;
    } else { // 3 or 4
      int row = i / 2;
      int col = i % 2;
      cardX = 15 + col * 150; cardY = 50 + row * 85; cardW = 140; cardH = 75;
    }

    // Save zone for touch detection
    zones[i].xMin = cardX;
    zones[i].xMax = cardX + cardW;
    zones[i].yMin = cardY;
    zones[i].yMax = cardY + cardH;
    zones[i].employeeIndex = empIdx;
    zoneCount++;

    // Color code based on check-in state
    uint16_t bgCol = emp.checkedIn ? COLOR_CHECKED_IN_BG : COLOR_CARD_BG;
    uint16_t borderCol = emp.checkedIn ? COLOR_CHECKED_IN_BORDER : COLOR_CARD_BORDER;

    tft.fillRoundRect(cardX, cardY, cardW, cardH, 8, bgCol);
    tft.drawRoundRect(cardX, cardY, cardW, cardH, 8, borderCol);

    // Draw card content
    int maxTextWidth = cardW - 16;
    
    // Layout for 1 card (Large display)
    if (activeCount == 1) {
      // Line 0: Employee Name
      tft.setTextSize(2);
      int nameWidth = emp.lines[0].length() * 12;
      int offset = marquee.position(nameWidth, maxTextWidth, currentMs, 2000);
      tft.setTextColor(COLOR_TEXT);
      tft.setCursor(cardX + 8 + (offset < 0 ? offset : (maxTextWidth - nameWidth) / 2), cardY + 25);
      tft.print(emp.lines[0]);

      // Line 1: Position
      tft.setTextSize(1);
      int posWidth = emp.lines[1].length() * 6;
      offset = marquee.position(posWidth, maxTextWidth, currentMs, 2000);
      tft.setTextColor(COLOR_TEXT_MUTED);
      tft.setCursor(cardX + 8 + (offset < 0 ? offset : (maxTextWidth - posWidth) / 2), cardY + 60);
      tft.print(emp.lines[1]);

      // Line 2: Shift Schedule
      int timeWidth = emp.lines[2].length() * 6;
      tft.setTextColor(COLOR_ACCENT);
      tft.setCursor(cardX + 8 + (maxTextWidth - timeWidth) / 2, cardY + 85);
      tft.print(emp.lines[2]);

      // Draw Check-in Button inside the card
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
      
    } 
    // Layout for 2 cards (Side-by-Side medium display)
    else if (activeCount == 2) {
      // Line 0: Employee Name
      tft.setTextSize(1); // Set size 1 for smaller width
      int nameWidth = emp.lines[0].length() * 6;
      int offset = marquee.position(nameWidth, maxTextWidth, currentMs, 2000);
      tft.setTextColor(COLOR_TEXT);
      tft.setCursor(cardX + 8 + (offset < 0 ? offset : 0), cardY + 20);
      tft.print(emp.lines[0]);

      // Line 1: Position
      int posWidth = emp.lines[1].length() * 6;
      offset = marquee.position(posWidth, maxTextWidth, currentMs, 2000);
      tft.setTextColor(COLOR_TEXT_MUTED);
      tft.setCursor(cardX + 8 + (offset < 0 ? offset : 0), cardY + 50);
      tft.print(emp.lines[1]);

      // Line 2: Shift Schedule
      tft.setTextColor(COLOR_ACCENT);
      tft.setCursor(cardX + 8, cardY + 80);
      tft.print(emp.lines[2]);

      // Draw Check-in Button inside the card
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
    } 
    // Layout for 3 or 4 cards (2x2 grid view)
    else {
      // Line 0: Employee Name
      tft.setTextSize(1);
      int nameWidth = emp.lines[0].length() * 6;
      int offset = marquee.position(nameWidth, maxTextWidth - 30, currentMs, 2000); // leave space for badge
      tft.setTextColor(COLOR_TEXT);
      tft.setCursor(cardX + 8 + (offset < 0 ? offset : 0), cardY + 12);
      tft.print(emp.lines[0]);

      // Line 1: Position
      int posWidth = emp.lines[1].length() * 6;
      offset = marquee.position(posWidth, maxTextWidth, currentMs, 2000);
      tft.setTextColor(COLOR_TEXT_MUTED);
      tft.setCursor(cardX + 8 + (offset < 0 ? offset : 0), cardY + 30);
      tft.print(emp.lines[1]);

      // Line 2: Shift Schedule
      tft.setTextColor(COLOR_ACCENT);
      tft.setCursor(cardX + 8, cardY + 48);
      tft.print(emp.lines[2]);

      // Draw Small Status Indicator Badge in bottom-right corner
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

    // 5. Visual Masking: Clean up text that bleeds out of the card limits due to scrolling.
    // Mask inside margins (left/right) with the card's background color.
    tft.fillRect(cardX + 1, cardY + 2, 7, cardH - 4, bgCol);
    tft.fillRect(cardX + cardW - 8, cardY + 2, 7, cardH - 4, bgCol);

    // Re-render rounded card borders to ensure they look sharp.
    tft.drawRoundRect(cardX, cardY, cardW, cardH, 8, borderCol);
  }

  // Mask screen area outside cards to handle overflow bleeding
  // Erase left edge margin of display
  tft.fillRect(0, 31, 15, cfg.height - 31, COLOR_BG);
  // Erase right edge margin of display
  tft.fillRect(305, 31, 15, cfg.height - 31, COLOR_BG);
  
  if (activeCount == 2) {
    // Erase area between the two cards
    tft.fillRect(155, 31, 10, cfg.height - 31, COLOR_BG);
  } else if (activeCount >= 3) {
    // Erase center vertical and horizontal spacing channels
    tft.fillRect(155, 31, 10, cfg.height - 31, COLOR_BG);
    tft.fillRect(0, 125, cfg.width, 10, COLOR_BG);
  }
}

void UIManager::renderDebug(bool rtcOk, bool wifiOk, int rssi,
                           const String& ssid, const String& ip) {
  // Header bar for debug screen
  tft.fillRect(0, 0, cfg.width, 30, COLOR_HEADER);
  tft.drawFastHLine(0, 30, cfg.width, COLOR_WIFI_ERR);
  
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(15, 11);
  tft.print("[WIFI DIAGNOSTICS & SYSTEM STATUS]");

  // Body background
  tft.fillRect(0, 31, cfg.width, cfg.height - 31, COLOR_BG);

  // Diagnostic items
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

void UIManager::handleTouch(int16_t x, int16_t y, EmployeeStore& store) {
  for (int i = 0; i < zoneCount; i++) {
    if (x >= zones[i].xMin && x <= zones[i].xMax &&
        y >= zones[i].yMin && y <= zones[i].yMax) {
      int idx = zones[i].employeeIndex;
      if (idx >= 0 && idx < store.size()) {
        Employee& emp = store.getMutable(idx);
        emp.checkedIn = !emp.checkedIn;
        Serial.printf("UI: Card touched. %s check-in status toggled to %s\r\n", 
                      emp.lines[0].c_str(), emp.checkedIn ? "CHECKED IN" : "CHECKED OUT");
        break;
      }
    }
  }
}
