# ESP32 OJT Display Sign

ESP32-powered LCD sign that shows employee schedules pulled from a Supabase REST API. Uses an ST7920 128x64 display and DS1302 RTC.

## Hardware

### ST7920 LCD (Software SPI)

| Pin | GPIO |
|-----|------|
| CLK | 23 |
| DAT | 22 |
| CS  | 21 |
| RST | 4 |

### DS1302 RTC (ThreeWire)

| Pin | GPIO |
|-----|------|
| DAT | 18 |
| CLK | 19 |
| RST | 5 |

## Quick Start

```bash
brew install platformio
cp .example.env .env    # then edit with your Wi-Fi + Supabase credentials
pio run -t upload
```

## Configuration

All secrets go in `.env` (never committed). See `.example.env` for all available options:

```bash
WIFI_SSID=YourNetwork
WIFI_PASSWORD=YourPassword
SUPABASE_URL=https://your-project.supabase.co/rest/v1/Employees
SUPABASE_KEY=your_key

# Optional: override Supabase column names (defaults match the schema)
# SUPABASE_FIELD_FIRST_NAME=first_name
# SUPABASE_FIELD_LAST_NAME=last_name
# SUPABASE_FIELD_POSITION=position
# SUPABASE_FIELD_START=schedule_start
# SUPABASE_FIELD_END=schedule_end
```

The device connects to Wi-Fi on boot, fetches employee schedules from Supabase every 30s, and rotates through them on the LCD.

## HTTP API

| Endpoint | Description |
|----------|-------------|
| `GET /status` | Device health (WiFi RSSI, RTC, employee count, debug mode) |
| `GET /debug` | Toggle diagnostic overlay on LCD |

## LCD Layout

```
[clk] HH:MM:SS           [wifi]
───────────────────────────────
Employee Name (bold, scrolls)
Position / Detail (scrolls)
Schedule Time (scrolls)
```

## Emulator

```bash
cd emulator && mkdir build && cd build && cmake .. && make && ./emulator
```

| Key | Action |
|-----|--------|
| `Space` | Toggle debug |
| `W` / `R` / `S` | Toggle WiFi / RTC / cycle signal |
| `E` | Edit lines |
| `Q` / `Esc` | Quit |

## Project Structure

```
├── src/                # Firmware
│   ├── main.cpp        # Entry point (App instance + setup/loop)
│   ├── app.h/cpp       # App orchestator — owns all components
│   ├── config.h        # DisplayConfig, SupabaseFieldMapping
│   ├── display.h/cpp   # DisplayRenderer + MarqueeEngine + icons
│   ├── network.h/cpp   # WifiManager (connect, reconnect)
│   ├── rtc.h/cpp       # RtcManager (DS1302 wrapper)
│   ├── employee.h/cpp  # Employee struct + EmployeeStore
│   └── supabase.h/cpp  # SupabaseClient (HTTPS fetch + parse)
├── emulator/           # SDL testbed
├── scripts/            # flash.sh, load_env.py
├── u8g2/               # Graphics lib (submodule)
├── .env                # Credentials (gitignored)
├── .example.env        # Template
└── platformio.ini
```

## License

MIT
