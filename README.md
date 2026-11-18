# 🌦️ ESP32 Remote Weather Station

A WiFi-connected climate station built on the ESP32. It reads temperature, humidity, pressure, altitude, air quality, rain, soil moisture and ambient light, and serves a live dashboard from the board itself. No cloud, no app, no external server. Open the board's IP in any browser on your network and watch the numbers update.

![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![Framework](https://img.shields.io/badge/framework-Arduino-00979D)
![License](https://img.shields.io/badge/license-MIT-green)

<!-- Add a screenshot or GIF of the dashboard here: ![Dashboard](docs/dashboard.png) -->

## ✨ Features

- **Self-hosted dashboard**: the HTML/CSS/JS lives in the firmware (`PROGMEM`) and is served by the ESP32 itself.
- **Live updates**: the page polls a JSON endpoint every 3 seconds and shows a connection indicator.
- **8 live readings**: temperature, humidity, pressure, altitude, air quality, rain, soil moisture and day/night.
- **Human-readable states**: raw ADC values are turned into labels such as *Good / Moderate / Poor*, *Heavy rain / Light rain / No rain* and *Dry / Moist / Wet*.
- **Night indicator LED**: an external LED blinks when the LDR detects darkness.
- **Graceful fallbacks**: if the BMP280 is missing, the dashboard falls back to the DHT11 temperature and shows `N/A` for pressure and altitude.
- **Noise reduction**: every analog reading is averaged over 16 samples.
- **Simple JSON API**: `/data` can feed Home Assistant, Grafana, Node-RED, scripts and so on.

## 🧰 Hardware

| Component | Purpose | ESP32 pin |
|---|---|---|
| ESP32 dev board | Microcontroller + WiFi + web server | n/a |
| DHT11 (or DHT22) | Humidity (and backup temperature) | GPIO 4 |
| BMP280 (I²C) | Temperature, pressure, altitude | SDA: GPIO 21, SCL: GPIO 22 |
| LDR + resistor divider | Ambient light (day/night) | GPIO 34 |
| Analog gas sensor module | Air quality | GPIO 35 |
| Analog rain sensor module | Rain detection | GPIO 32 |
| Soil moisture sensor | Soil moisture | GPIO 33 |
| LED + 220/330 Ω resistor | Night indicator | GPIO 25 |

> All analog sensors are on **ADC1 pins (32–35)**, so they keep working while WiFi is active. Avoid ADC2 pins for analog reads in WiFi projects.

## 🔌 Wiring notes

- Power the BMP280 from **3.3 V**. The BMP280 address is auto-detected (`0x76` or `0x77`).
- Check whether your gas, rain and soil modules are happy on 3.3 V. Some need 5 V, in which case the analog output must not exceed 3.3 V at the ESP32 pin.
- Put the LED in series with a 220–330 Ω resistor to GND.

## 📦 Software requirements

- [Arduino IDE](https://www.arduino.cc/en/software) 2.x (or PlatformIO)
- ESP32 board package by Espressif (Boards Manager)
- Libraries (Library Manager):
  - `DHT sensor library` by Adafruit
  - `Adafruit BMP280 Library`
  - `Adafruit Unified Sensor` (dependency)
- Built-in with the ESP32 core: `WiFi`, `WebServer`, `Wire`

## 🚀 Getting started

1. **Clone the repo**
   ```bash
   git clone https://github.com/<your-username>/<your-repo>.git
   ```
2. **Add your WiFi credentials.** Copy `secrets.example.h` to `secrets.h` and fill it in:
   ```cpp
   #define WIFI_SSID "your-network-name"
   #define WIFI_PASS "your-password"
   ```
   `secrets.h` is git-ignored, so your credentials never get pushed.
3. **Select the board**: *Tools → Board → ESP32 Dev Module* and choose the right COM port.
4. **Upload** the sketch and open the Serial Monitor at **115200 baud**.
5. **Open the dashboard.** Once WiFi connects, the serial output prints:
   ```
   Open dashboard at: http://192.168.x.x
   ```
   Type that address into a browser on the same network.

## 🌐 HTTP endpoints

| Route | Description |
|---|---|
| `GET /` | Live dashboard (HTML) |
| `GET /data` | Current readings as JSON (CORS enabled) |

Example `/data` response:

```json
{
  "time_of_day": "Daylight",
  "ldr_raw": 820,
  "gas_raw": 950,
  "gas_quality": "Good",
  "bmp_temperature": 27.4,
  "dht_temperature": 27.0,
  "humidity": 58.0,
  "pressure_hpa": 912.3,
  "altitude_m": 890,
  "rain_raw": 4095,
  "rain_state": "No rain",
  "soil_raw": 2400,
  "soil_percent": 42,
  "soil_state": "Moist"
}
```

A sensor that fails to read is returned as `null`.

## 🎛️ Calibration

Every analog sensor behaves differently, so tune these constants at the top of the sketch for your hardware:

| Constant | Meaning |
|---|---|
| `LDR_BRIGHT_IS_HIGH` / `LDR_DAY_THRESHOLD` | LDR direction and the day/night cutoff |
| `GAS_GOOD_MAX` / `GAS_MODERATE_MAX` | Raw limits for Good / Moderate / Poor air |
| `RAIN_HEAVY_MAX` / `RAIN_LIGHT_MAX` | Raw limits for heavy / light / no rain |
| `SOIL_DRY_RAW` / `SOIL_WET_RAW` | Raw value in dry air vs. in water, used to compute the moisture % |
| `SEA_LEVEL_PRESSURE_HPA` | Set to your local sea-level pressure for accurate altitude |
| `DHT_TYPE` | `DHT11` or `DHT22` |

**Tip:** print the raw values to the Serial Monitor (or read them from the dashboard) with the sensor in known conditions (dry air, wet soil, a dark room) and use those numbers as your thresholds.

## 🛠️ Troubleshooting

- **Stuck on "Connecting to WiFi..."**: the ESP32 only supports 2.4 GHz networks. Check the SSID and password.
- **"BMP280 not found"**: check SDA/SCL wiring and power. Try an I²C scanner sketch.
- **DHT shows "Error"**: check the data pin and whether `DHT_TYPE` matches your sensor.
- **Dashboard says "Connection lost"**: the board and your device must be on the same network. Check the IP in the Serial Monitor.

## 🗺️ Roadmap

- [ ] Historical charts (last 24 h)
- [ ] MQTT / Home Assistant integration
- [ ] Display an IP-entry field so one dashboard can connect to multiple stations
- [ ] Deep sleep + battery/solar mode
- [ ] OTA firmware updates
- [ ] Data logging to SD card or a time-series database
- [ ] Dark/light theme toggle

## 🤝 Contributing

Issues and pull requests are welcome. If you build your own version, open an issue and share a photo of your setup.

## 📄 License

Released under the [MIT License](LICENSE).

## 🙌 Acknowledgements

- [Adafruit](https://github.com/adafruit) for the DHT and BMP280 libraries
- The Espressif Arduino-ESP32 community
