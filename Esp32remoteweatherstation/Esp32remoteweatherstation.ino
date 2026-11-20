#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_BMP280.h>

// ---------- Your settings ----------
const char* WIFI_SSID = "Wifi name";
const char* WIFI_PASS = "Password of your wifi";

#define DHT_PIN   4
#define DHT_TYPE  DHT11          // change to DHT22 if you have that one
#define LDR_PIN   34
#define GAS_PIN   35
#define RAIN_PIN  32
#define SOIL_PIN  33
#define LED_PIN   25             // External LED pin (via 220/330 ohm resistor to GND)

// I2C Pins for BMP280
#define I2C_SDA   21
#define I2C_SCL   22
#define SEA_LEVEL_PRESSURE_HPA 1013.25

// ---------- Calibration ----------
const bool LDR_BRIGHT_IS_HIGH = false;  // false = lower ADC value in bright light, higher in dark
const int  LDR_DAY_THRESHOLD  = 1500;

const int GAS_GOOD_MAX     = 1200;
const int GAS_MODERATE_MAX = 2200;

const int RAIN_HEAVY_MAX = 1500;
const int RAIN_LIGHT_MAX = 3000;

const int SOIL_DRY_RAW = 3200;
const int SOIL_WET_RAW = 1300;

// ---------- LED Blink Timing ----------
const unsigned long BLINK_INTERVAL_MS = 500;
unsigned long lastBlinkMillis = 0;
bool ledState = LOW;

DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_BMP280 bmp;
bool bmpAvailable = false;

WebServer server(80);

// ---------- Embedded Web Dashboard ----------
const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Weather Station</title>
  <style>
    :root {
      --bg: #0f172a;
      --card: #1e293b;
      --text: #f8fafc;
      --muted: #94a3b8;
      --accent: #38bdf8;
      --good: #4ade80;
      --warn: #facc15;
      --bad: #f87171;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
    body { background: var(--bg); color: var(--text); padding: 20px; display: flex; flex-direction: column; align-items: center; }
    header { text-align: center; margin-bottom: 24px; }
    h1 { font-size: 1.6rem; font-weight: 700; letter-spacing: -0.5px; }
    .status { font-size: 0.85rem; color: var(--muted); margin-top: 6px; }
    .dot { height: 8px; width: 8px; background: var(--good); border-radius: 50%; display: inline-block; margin-right: 6px; }
    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(170px, 1fr));
      gap: 16px;
      width: 100%;
      max-width: 760px;
    }
    .card {
      background: var(--card);
      border-radius: 16px;
      padding: 20px;
      box-shadow: 0 4px 12px rgba(0,0,0,0.25);
      display: flex;
      flex-direction: column;
      justify-content: space-between;
    }
    .label { font-size: 0.85rem; color: var(--muted); text-transform: uppercase; letter-spacing: 0.5px; }
    .value { font-size: 1.9rem; font-weight: 700; margin: 10px 0 4px; color: var(--accent); }
    .sub { font-size: 0.85rem; color: var(--muted); }
    .badge {
      display: inline-block;
      padding: 4px 10px;
      border-radius: 999px;
      font-size: 0.8rem;
      font-weight: 600;
      margin-top: 8px;
      width: fit-content;
      background: rgba(56, 189, 248, 0.15);
      color: var(--accent);
    }
  </style>
</head>
<body>
  <header>
    <h1>Climate Weather Station</h1>
    <div class="status"><span class="dot" id="dot"></span><span id="updated">Connecting...</span></div>
  </header>

  <div class="grid">
    <div class="card">
      <div class="label">Temperature</div>
      <div class="value" id="temp">-- °C</div>
      <div class="sub">BMP280 Sensor</div>
    </div>

    <div class="card">
      <div class="label">Humidity</div>
      <div class="value" id="hum">-- %</div>
      <div class="sub">DHT11 Sensor</div>
    </div>

    <div class="card">
      <div class="label">Pressure</div>
      <div class="value" id="press">-- hPa</div>
      <div class="sub">BMP280 Barometer</div>
    </div>

    <div class="card">
      <div class="label">Altitude</div>
      <div class="value" id="alt">-- m</div>
      <div class="sub">Est. Above Sea Level</div>
    </div>

    <div class="card">
      <div class="label">Air Quality</div>
      <div class="value" id="gas-state">--</div>
      <div class="sub">Raw: <span id="gas-raw">--</span></div>
    </div>

    <div class="card">
      <div class="label">Weather / Rain</div>
      <div class="value" id="rain-state">--</div>
      <div class="sub">Raw: <span id="rain-raw">--</span></div>
    </div>

    <div class="card">
      <div class="label">Soil Moisture</div>
      <div class="value" id="soil-pct">-- %</div>
      <div class="badge" id="soil-state">--</div>
    </div>

    <div class="card">
      <div class="label">Ambient Light</div>
      <div class="value" id="time-day">--</div>
      <div class="sub">LDR Raw: <span id="ldr-raw">--</span></div>
    </div>
  </div>

  <script>
    async function fetchReadings() {
      try {
        const res = await fetch('/data');
        const d = await res.json();

        // Use BMP280 temperature as primary; fall back to DHT11 if BMP280 is unavailable
        const primaryTemp = d.bmp_temperature !== null ? d.bmp_temperature : d.dht_temperature;
        document.getElementById('temp').textContent = primaryTemp !== null ? `${primaryTemp} °C` : 'Error';
        document.getElementById('hum').textContent  = d.humidity !== null ? `${d.humidity} %` : 'Error';

        document.getElementById('press').textContent = d.pressure_hpa !== null ? `${d.pressure_hpa} hPa` : 'N/A';
        document.getElementById('alt').textContent   = d.altitude_m !== null ? `${d.altitude_m} m` : 'N/A';

        const gasEl = document.getElementById('gas-state');
        gasEl.textContent = d.gas_quality;
        gasEl.style.color = d.gas_quality === 'Good' ? 'var(--good)' : (d.gas_quality === 'Moderate' ? 'var(--warn)' : 'var(--bad)');
        document.getElementById('gas-raw').textContent = d.gas_raw;

        document.getElementById('rain-state').textContent = d.rain_state;
        document.getElementById('rain-raw').textContent   = d.rain_raw;

        document.getElementById('soil-pct').textContent   = `${d.soil_percent}%`;
        document.getElementById('soil-state').textContent = `${d.soil_state} (Raw: ${d.soil_raw})`;

        document.getElementById('time-day').textContent = d.time_of_day;
        document.getElementById('ldr-raw').textContent  = d.ldr_raw;

        document.getElementById('dot').style.background = 'var(--good)';
        document.getElementById('updated').textContent = 'Live • Updated ' + new Date().toLocaleTimeString();
      } catch (e) {
        document.getElementById('dot').style.background = 'var(--bad)';
        document.getElementById('updated').textContent = 'Connection lost — retrying...';
      }
    }

    fetchReadings();
    setInterval(fetchReadings, 3000);
  </script>
</body>
</html>
)rawliteral";

int readAveraged(int pin) {
  long sum = 0;
  for (int i = 0; i < 16; i++) {
    sum += analogRead(pin);
    delay(2);
  }
  return sum / 16;
}

bool checkIsDay(int ldrValue) {
  return LDR_BRIGHT_IS_HIGH ? (ldrValue > LDR_DAY_THRESHOLD) : (ldrValue < LDR_DAY_THRESHOLD);
}

void handleDarknessLed() {
  int ldr = analogRead(LDR_PIN);
  bool isDay = checkIsDay(ldr);

  if (!isDay) {
    unsigned long now = millis();
    if (now - lastBlinkMillis >= BLINK_INTERVAL_MS) {
      lastBlinkMillis = now;
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
    }
  } else {
    if (ledState != LOW) {
      ledState = LOW;
      digitalWrite(LED_PIN, LOW);
    }
  }
}

void sendJson() {
  int ldr  = readAveraged(LDR_PIN);
  int gas  = readAveraged(GAS_PIN);
  int rain = readAveraged(RAIN_PIN);
  int soil = readAveraged(SOIL_PIN);

  float dhtTemp = dht.readTemperature();
  float h       = dht.readHumidity();

  float bmpTemp  = NAN;
  float pressHpa = NAN;
  float altM     = NAN;

  if (bmpAvailable) {
    bmpTemp  = bmp.readTemperature();
    pressHpa = bmp.readPressure() / 100.0F;
    altM     = bmp.readAltitude(SEA_LEVEL_PRESSURE_HPA);
  }

  bool isDay = checkIsDay(ldr);

  const char* gasQuality = gas < GAS_GOOD_MAX ? "Good" : (gas < GAS_MODERATE_MAX ? "Moderate" : "Poor");
  const char* rainState  = rain < RAIN_HEAVY_MAX ? "Heavy rain" : (rain < RAIN_LIGHT_MAX ? "Light rain" : "No rain");

  int soilPct = map(soil, SOIL_DRY_RAW, SOIL_WET_RAW, 0, 100);
  soilPct = constrain(soilPct, 0, 100);
  const char* soilState = soilPct < 30 ? "Dry" : (soilPct < 60 ? "Moist" : "Wet");

  String json = "{";
  json += "\"time_of_day\":\"" + String(isDay ? "Daylight" : "Night") + "\",";
  json += "\"ldr_raw\":" + String(ldr) + ",";
  json += "\"gas_raw\":" + String(gas) + ",";
  json += "\"gas_quality\":\"" + String(gasQuality) + "\",";
  json += "\"bmp_temperature\":" + (isnan(bmpTemp) ? String("null") : String(bmpTemp, 1)) + ",";
  json += "\"dht_temperature\":" + (isnan(dhtTemp) ? String("null") : String(dhtTemp, 1)) + ",";
  json += "\"humidity\":" + (isnan(h) ? String("null") : String(h, 1)) + ",";
  json += "\"pressure_hpa\":" + (isnan(pressHpa) ? String("null") : String(pressHpa, 1)) + ",";
  json += "\"altitude_m\":" + (isnan(altM) ? String("null") : String(altM, 0)) + ",";
  json += "\"rain_raw\":" + String(rain) + ",";
  json += "\"rain_state\":\"" + String(rainState) + "\",";
  json += "\"soil_raw\":" + String(soil) + ",";
  json += "\"soil_percent\":" + String(soilPct) + ",";
  json += "\"soil_state\":\"" + String(soilState) + "\"";
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void sendDashboard() {
  server.send_P(200, "text/html", DASHBOARD_HTML);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  analogReadResolution(12);
  dht.begin();

  Wire.begin(I2C_SDA, I2C_SCL);
  if (bmp.begin(0x76) || bmp.begin(0x77)) {
    bmpAvailable = true;
    Serial.println("BMP280 initialized successfully.");
  } else {
    Serial.println("Warning: BMP280 not found! Check SDA (21) and SCL (22) wiring.");
  }

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Open dashboard at: http://");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, sendDashboard);
  server.on("/data", HTTP_GET, sendJson);
  server.begin();
}

void loop() {
  server.handleClient();
  handleDarknessLed();
}
