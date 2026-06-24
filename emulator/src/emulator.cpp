#include <SDL2/SDL.h>
#include <u8g2.h>
#include <cstring>
#include <cstdio>
#include <ctime>

// ============================================================================
// GRAPHICAL RENDERING HELPERS (Adapted from Arduino code)
// ============================================================================

/**
 * Draws a pixel-perfect, diagonal 3-bar Wi-Fi signal icon.
 */
void drawWifiIcon(u8g2_t *u8g2, int x, int y, int rssi, bool connected) {
  if (!connected) {
    u8g2_DrawLine(u8g2, x, y + 1, x + 4, y + 5);
    u8g2_DrawLine(u8g2, x + 4, y + 1, x, y + 5);
    return;
  }
  // Bar 1 (2px height)
  u8g2_DrawLine(u8g2, x, y + 4, x, y + 5);
  // Bar 2 (4px height: >= -80 dBm)
  if (rssi >= -80) {
    u8g2_DrawLine(u8g2, x + 2, y + 2, x + 2, y + 5);
  }
  // Bar 3 (6px height: >= -70 dBm)
  if (rssi >= -70) {
    u8g2_DrawLine(u8g2, x + 4, y, x + 4, y + 5);
  }
}

/**
 * Draws a pixel-perfect 7x7 micro status indicator.
 * Displays a clean clock face if connected, or blanks out the area and renders
 * a crisp standalone "X" if disconnected.
 */
void drawClockIcon(u8g2_t *u8g2, int x, int y, bool rtcConnected) {
  if (rtcConnected) {
    // Draw outer circle frame with diameter of 7 pixels (radius 3)
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawCircle(u8g2, x + 3, y + 3, 3, U8G2_DRAW_ALL);
    
    // Center point
    u8g2_DrawPixel(u8g2, x + 3, y + 3);
    
    // Clean, proportioned clock hands
    u8g2_DrawPixel(u8g2, x + 3, y + 2); // vertical hand (Minute) - facing UP
    u8g2_DrawPixel(u8g2, x + 2, y + 3); // horizontal hand (Hour) - facing LEFT (corrected)
  } else {
    // 1. Draw the complete clock face circle and hands first
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawCircle(u8g2, x + 3, y + 3, 3, U8G2_DRAW_ALL);
    u8g2_DrawPixel(u8g2, x + 3, y + 3);
    u8g2_DrawPixel(u8g2, x + 3, y + 2); // vertical hand
    u8g2_DrawPixel(u8g2, x + 2, y + 3); // left hand

    // 2. Blank out surrounding pixels of the 'X' to create a visual separation gap
    // This turns off any pixels that would touch the "X" structure
    u8g2_SetDrawColor(u8g2, 0); 
    u8g2_DrawBox(u8g2, x + 4, y + 4, 3, 3); // Clear the bottom-right quadrant of the clock
    u8g2_DrawPixel(u8g2, x + 3, y + 4); // Left adjacent
    u8g2_DrawPixel(u8g2, x + 4, y + 3); // Top adjacent
    u8g2_DrawPixel(u8g2, x + 3, y + 5); // Diagonal adjacent left
    u8g2_DrawPixel(u8g2, x + 5, y + 3); // Diagonal adjacent top
    u8g2_DrawPixel(u8g2, x + 3, y + 6); // Bottom border adjacent
    u8g2_DrawPixel(u8g2, x + 6, y + 3); // Right border adjacent

    // 3. Render a crisp standalone visual "X" in the bottom-right corner
    u8g2_SetDrawColor(u8g2, 1); 
    u8g2_DrawPixel(u8g2, x + 4, y + 4);
    u8g2_DrawPixel(u8g2, x + 6, y + 4);
    u8g2_DrawPixel(u8g2, x + 5, y + 5);
    u8g2_DrawPixel(u8g2, x + 4, y + 6);
    u8g2_DrawPixel(u8g2, x + 6, y + 6);
  }
}

// ============================================================================
// MAIN SDL EMULATOR
// ============================================================================

int main(int argc, char **argv) {
  // Initialize U8g2 with SDL
  u8g2_t u8g2;
  u8g2_SetupBuffer_SDL_128x64(&u8g2, &u8g2_cb_r0);
  u8g2_InitDisplay(&u8g2);
  u8g2_SetPowerSave(&u8g2, 0);

  // SDL event loop
  bool running = true;
  SDL_Event event;
  
  // Test state variables
  bool debugMode = false;
  bool isWifiConnected = true;
  bool isRtcConnected = true;
  int currentRssi = -65; // Strong signal
  int stateIndex = 0;
  bool editMode = false;
  int editingLine = 0;
  
  // Display text (matching Arduino default)
  char displayLines[3][64] = {
    "Jose Daniel Gamboa Percy",
    "Technical Assistant III",
    "DOST Regional Office"
  };

  // Timing
  Uint32 lastRefresh = 0;
  const Uint32 refreshInterval = 1000; // 1 second refresh

  printf("SDL Emulator Started\n");
  printf("Press SPACE to toggle debug mode\n");
  printf("Press 'W' to toggle WiFi connection\n");
  printf("Press 'R' to toggle RTC connection\n");
  printf("Press 'S' to cycle through signal strengths\n");
  printf("Press 'E' to enter edit mode for display lines\n");
  printf("In edit mode: 1/2/3 to select line, BACKSPACE to delete, ENTER to exit\n");
  printf("Press ESC or Q to quit\n");

  while (running) {
    // Handle SDL events
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      } else if (event.type == SDL_KEYDOWN) {
        if (editMode) {
          // Edit mode key handling
          switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
              editMode = false;
              printf("Edit mode exited\n");
              break;
            case SDLK_1:
              editingLine = 0;
              printf("Editing line 1\n");
              break;
            case SDLK_2:
              editingLine = 1;
              printf("Editing line 2\n");
              break;
            case SDLK_3:
              editingLine = 2;
              printf("Editing line 3\n");
              break;
            case SDLK_BACKSPACE:
              {
                int len = strlen(displayLines[editingLine]);
                if (len > 0) {
                  displayLines[editingLine][len - 1] = '\0';
                }
              }
              break;
            default:
              // Handle printable characters
              if (event.key.keysym.sym >= SDLK_SPACE && event.key.keysym.sym <= SDLK_z) {
                int len = strlen(displayLines[editingLine]);
                if (len < 63) {
                  char c = event.key.keysym.sym;
                  // Handle uppercase with shift
                  if (event.key.keysym.mod & KMOD_SHIFT) {
                    if (c >= 'a' && c <= 'z') c -= 32;
                  }
                  displayLines[editingLine][len] = c;
                  displayLines[editingLine][len + 1] = '\0';
                }
              }
              break;
          }
        } else {
          // Normal mode key handling
          switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
            case SDLK_q:
              running = false;
              break;
            case SDLK_SPACE:
              debugMode = !debugMode;
              printf("Debug mode: %s\n", debugMode ? "ON" : "OFF");
              break;
            case SDLK_w:
              isWifiConnected = !isWifiConnected;
              printf("WiFi: %s\n", isWifiConnected ? "Connected" : "Disconnected");
              break;
            case SDLK_r:
              isRtcConnected = !isRtcConnected;
              printf("RTC: %s\n", isRtcConnected ? "Connected" : "Disconnected");
              break;
            case SDLK_s:
              stateIndex = (stateIndex + 1) % 4;
              switch (stateIndex) {
                case 0: currentRssi = -65; printf("Signal: Strong (-65 dBm)\n"); break;
                case 1: currentRssi = -75; printf("Signal: Good (-75 dBm)\n"); break;
                case 2: currentRssi = -85; printf("Signal: Weak (-85 dBm)\n"); break;
                case 3: currentRssi = -95; printf("Signal: Very Weak (-95 dBm)\n"); break;
              }
              break;
            case SDLK_e:
              editMode = true;
              editingLine = 0;
              printf("Edit mode entered - editing line 1\n");
              break;
          }
        }
      }
    }

    // Refresh display at interval
    Uint32 currentTicks = SDL_GetTicks();
    if (currentTicks - lastRefresh >= refreshInterval) {
      lastRefresh = currentTicks;

      // Get current time for RTC display
      char timeBuffer[9] = "--:--:--";
      if (isRtcConnected) {
        time_t now = time(nullptr);
        struct tm *tm_info = localtime(&now);
        strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", tm_info);
      }

      // Clear buffer
      u8g2_ClearBuffer(&u8g2);

      if (debugMode) {
        u8g2_DrawFrame(&u8g2, 0, 0, 128, 64);
        u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
        u8g2_DrawStr(&u8g2, 6, 12, "[WiFi Diagnostic]");
        u8g2_DrawHLine(&u8g2, 4, 15, 120);

        char buffer[64];
        u8g2_DrawStr(&u8g2, 6, 28, "SSID: DOST6-DRRM");

        if (isWifiConnected) {
          u8g2_DrawStr(&u8g2, 6, 40, "IP  : 192.168.1.100");
        } else {
          u8g2_DrawStr(&u8g2, 6, 40, "IP  : Disconnected");
        }

        snprintf(buffer, sizeof(buffer), "RSSI: %d dBm | RTC: %s", 
                 isWifiConnected ? currentRssi : 0,
                 isRtcConnected ? "OK" : "ERR");
        u8g2_DrawStr(&u8g2, 6, 52, buffer);
      } else {
        u8g2_DrawFrame(&u8g2, 0, 0, 128, 64);

        // Render top status bar components
        drawClockIcon(&u8g2, 6, 6, isRtcConnected);
        drawWifiIcon(&u8g2, 116, 6, currentRssi, isWifiConnected);

        // Only display the actual time if the RTC module is connected & validated
        if (isRtcConnected) {
          u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);     
          u8g2_DrawStr(&u8g2, 16, 14, timeBuffer);
        }
        
        u8g2_DrawHLine(&u8g2, 4, 17, 120);

        // Dynamic Loop String Rendering
        for (int i = 0; i < 3; i++) {
          if (strlen(displayLines[i]) > 0) {
            if (i == 0) {
              u8g2_SetFont(&u8g2, u8g2_font_helvB10_tf);
              u8g2_DrawStr(&u8g2, 6, 32, displayLines[i]);
            } else {
              u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
              int y_pos = (i == 1) ? 46 : 57;
              u8g2_DrawStr(&u8g2, 6, y_pos, displayLines[i]);
            }
          }
        }
      }
      
      u8g2_SendBuffer(&u8g2);
    }

    // Small delay to prevent CPU spinning
    SDL_Delay(10);
  }

  printf("Emulator shutdown\n");
  SDL_Quit();
  return 0;
}
