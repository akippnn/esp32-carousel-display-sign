# ESP32 OJT Display Sign

ESP32-powered LCD sign that shows employee schedules pulled from a Supabase REST API. Uses an ST7920 128x64 display and DS1302 RTC.

> [!NOTE]
> **Legacy Hardware (ST7920):** The older version of this project configured for the monochrome ST7920 display (128x64) is preserved in the `ST7920` branch.

## Hardware Configuration

The current version uses a 3.2" color ILI9341 TFT display with an integrated XPT2046 resistive touch controller, alongside the DS1302 Real-Time Clock. All SPI devices share the clock and MOSI lines, mapped via the ESP32 GPIO matrix.

### ESP32 Pin Map

| Subsystem | Signal Name | ESP32 GPIO | Header Pin | Description |
| :--- | :--- | :---: | :---: | :--- |
| **SPI Bus (Shared)** | `SPI_CLK` | **IO5** | J2_10 | Shared clock line (TFT_SCK & T_CLK bridged at header) |
| | `SPI_MOSI` | **IO17** | J2_11 | Shared data out line (TFT_MOSI & T_DIN bridged at header) |
| | `T_DO` (MISO) | **IO26** | J1_10 | Shared data in line (used by Touch Controller) |
| **ILI9341 TFT** | `TFT_CS` | **IO15** | J2_16 | TFT Chip Select |
| | `TFT_DC` | **IO4** | J2_13 | TFT Data/Command Control |
| | `TFT_RST` | **IO2** | J2_15 | TFT Hardware Reset |
| **XPT2046 Touch** | `T_CS` | **IO27** | J1_11 | Touch Controller Chip Select |
| | `T_IRQ` | **IO35** | J1_6 | Touch Interrupt (goes LOW when screen is touched) |
| **DS1302 RTC** | `RTC_CLK` | **IO25** | J1_9 | RTC Serial Clock |
| | `RTC_RST` | **IO32** | J1_7 | RTC Reset / Chip Enable |
| | `RTC_DAT` | **IO33** | J1_8 | RTC ThreeWire I/O Data Line |

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
