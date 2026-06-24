#ifndef DISPLAY_H
#define DISPLAY_H

void drawWifiIcon(int x, int y, int rssi, bool connected);
void drawClockIcon(int x, int y, bool rtcConnected);
int marqueeX(int textWidth, int maxWidth, unsigned long nowMs, int lineIndex, unsigned long maxFwdMs);

#endif
