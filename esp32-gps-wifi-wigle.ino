/*
 * esp32-gps-wifi-wigle — GPS + WiFi wardriving logger for WiGLE.net
 *
 * Required libraries (Arduino IDE Library Manager or arduino-cli lib install):
 *   TinyGPSPlus          by Mikal Hart     >= 1.0.3
 *   Adafruit GFX Library by Adafruit       >= 1.11.9
 *   Adafruit SSD1306     by Adafruit       >= 2.5.9
 *   RTClib               by Adafruit       >= 2.1.4
 *
 * Built-in to ESP32 Arduino core (no separate install needed):
 *   WiFi, SD, SPI, Wire, HardwareSerial
 */

#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SD.h>
#include <time.h>
#include <RTClib.h>

// RTC setup
RTC_Millis rtc;

// Pin definitions
static const int RXPin = 16, TXPin = 17;
static const uint32_t GPSBaud = 9600;

// Display & hardware constants
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define SD_CS_PIN       5
#define LED_PIN         2     // built-in LED on most ESP32 DevKit boards
#define MIN_SATELLITES  4
#define PST_OFFSET      -8    // defined for reference; timestamps are logged as UTC
#define SCAN_INTERVAL_MS 15000

// Objects
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
File csvFile;

// State
int scanCount     = 0;
int networkCount  = 0;       // networks found in the most recent scan
int totalNetworks = 0;       // cumulative networks across all scans
char csvFilename[32] = "";   // timestamped filename persisted across functions
bool systemReady  = false;   // true only after all hardware initialises successfully

// ─── setup / loop ────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  gpsSerial.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin);
  delay(5000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Card failed, or not present");
    displayError("SD Card Error");
    digitalWrite(LED_PIN, HIGH);
    return;
  }

  if (!initializeCSV()) {
    displayError("CSV Init Error");
    digitalWrite(LED_PIN, HIGH);
    return;
  }

  rtc.begin(DateTime(F(__DATE__), F(__TIME__)));
  systemReady = true;
  Serial.println(F("GPS and WiFi Scanner ready"));
}

void loop() {
  if (!systemReady) return;

  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isValid() && gps.hdop.isValid() &&
      gps.date.isValid()     && gps.time.isValid() &&
      gps.satellites.value() >= MIN_SATELLITES) {

    DateTime gpsTime(gps.date.year(), gps.date.month(), gps.date.day(),
                     gps.time.hour(), gps.time.minute(), gps.time.second());
    rtc.adjust(gpsTime);

    displayGPSData();
    blinkLED(3, 100, 100);   // fast triple blink = scan starting
    scanWiFiNetworks();
    displayGPSData();
    delay(SCAN_INTERVAL_MS);
  } else {
    displaySearching();
  }
}

// ─── display helpers ─────────────────────────────────────────────────────────

void displayGPSData() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  display.print("Lat: ");
  display.println(gps.location.lat(), 6);
  display.print("Lng: ");
  display.println(gps.location.lng(), 6);
  display.print("Alt: ");
  display.println(gps.altitude.meters());

  // Combine sat count and speed on one line to keep the display to 7 rows
  display.print("Sat:");
  display.print(gps.satellites.value());
  display.print(" Spd:");
  if (gps.speed.isValid()) {
    display.print(gps.speed.kmph(), 1);
    display.println("k");
  } else {
    display.println("--");
  }

  DateTime now = rtc.now();
  char timebuf[20];
  snprintf(timebuf, sizeof(timebuf), "%04d-%02d-%02d %02d:%02d:%02d",
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());
  display.print("Time: ");
  display.println(timebuf);

  display.print("Nets: ");
  display.println(totalNetworks);
  display.print("Scans: ");
  display.println(scanCount);

  display.display();
}

void displaySearching() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Searching for GPS...");
  display.print("Satellites: ");
  display.println(gps.satellites.value());
  display.display();
  blinkLED(1, 500, 500);   // slow single blink = waiting for GPS fix
}

void displayError(const char* error) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(error);
  display.display();
}

// ─── utility helpers ─────────────────────────────────────────────────────────

void blinkLED(int count, int on_ms, int off_ms) {
  for (int i = 0; i < count; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(on_ms);
    digitalWrite(LED_PIN, LOW);
    delay(off_ms);
  }
}

// Correct 802.11 channel → frequency mapping for 2.4 GHz and 5 GHz
int channelToFrequency(int channel) {
  if (channel >= 1 && channel <= 13) return 2407 + channel * 5;
  if (channel == 14)                 return 2484;
  if (channel >= 36)                 return 5000 + channel * 5;
  return 0;
}

// WiGLE-compatible AuthMode string derived from the ESP32 encryption type
String getAuthMode(wifi_auth_mode_t authType) {
  switch (authType) {
    case WIFI_AUTH_OPEN:            return "[ESS]";
    case WIFI_AUTH_WEP:             return "[WEP][ESS]";
    case WIFI_AUTH_WPA_PSK:         return "[WPA-PSK-CCMP][ESS]";
    case WIFI_AUTH_WPA2_PSK:        return "[WPA2-PSK-CCMP][ESS]";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "[WPA-PSK-CCMP][WPA2-PSK-CCMP][ESS]";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "[WPA2-EAP-CCMP][ESS]";
    case WIFI_AUTH_WPA3_PSK:        return "[WPA3-SAE-CCMP][ESS]";
    case WIFI_AUTH_WPA2_WPA3_PSK:   return "[WPA2-PSK-CCMP][WPA3-SAE-CCMP][ESS]";
    default:                        return "[ESS]";
  }
}

// RFC 4180: double any embedded quotes; strip bare newlines
String sanitizeSSID(const String& ssid) {
  String out;
  out.reserve(ssid.length());
  for (int i = 0; i < (int)ssid.length(); i++) {
    char c = ssid.charAt(i);
    if (c == '"')            out += "\"\"";
    else if (c == '\n' || c == '\r') out += ' ';
    else                     out += c;
  }
  return out;
}

const char* hdopQuality(float hdop) {
  if (hdop <= 1.0f)  return "Ideal";
  if (hdop <= 2.0f)  return "Excellent";
  if (hdop <= 5.0f)  return "Good";
  if (hdop <= 10.0f) return "Moderate";
  if (hdop <= 20.0f) return "Fair";
  return "Poor";
}

String getLocalTimestamp() {
  DateTime now = rtc.now();
  char buf[20];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());
  return String(buf);
}

// ─── CSV / SD helpers ────────────────────────────────────────────────────────

bool initializeCSV() {
  DateTime now = rtc.now();
  snprintf(csvFilename, sizeof(csvFilename), "/wigle_%04d%02d%02d_%02d%02d%02d.csv",
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());

  csvFile = SD.open(csvFilename, FILE_WRITE);
  if (!csvFile) return false;

  // WiGLE 1.4 format: device descriptor line followed by column header line
  csvFile.println(
    "WigleWifi-1.4,appRelease=2.26,model=ESP32,release=1.0.0,"
    "device=ESP32-Wardriver,display=OLED,board=ESP32,brand=Espressif"
  );
  csvFile.println(
    "MAC,SSID,AuthMode,FirstSeen,Channel,Frequency,RSSI,"
    "CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,Type"
  );
  csvFile.close();
  return true;
}

void writeNetworkData(int idx) {
  String bssid    = WiFi.BSSIDstr(idx);
  String ssid     = sanitizeSSID(WiFi.SSID(idx));
  String authMode = getAuthMode(WiFi.encryptionType(idx));
  String ts       = getLocalTimestamp();
  int    channel  = WiFi.channel(idx);
  int    freq     = channelToFrequency(channel);
  int    rssi     = WiFi.RSSI(idx);

  csvFile.print("\""); csvFile.print(bssid);    csvFile.print("\",");
  csvFile.print("\""); csvFile.print(ssid);     csvFile.print("\",");
  csvFile.print("\""); csvFile.print(authMode); csvFile.print("\",");
  csvFile.print("\""); csvFile.print(ts);       csvFile.print("\",");
  csvFile.print(channel);   csvFile.print(",");
  csvFile.print(freq);      csvFile.print(",");
  csvFile.print(rssi);      csvFile.print(",");
  csvFile.print(gps.location.lat(), 6); csvFile.print(",");
  csvFile.print(gps.location.lng(), 6); csvFile.print(",");
  csvFile.print(gps.altitude.meters()); csvFile.print(",");
  csvFile.print(gps.hdop.hdop());       csvFile.print(",");
  csvFile.println("WIFI");
}

void scanWiFiNetworks() {
  networkCount = WiFi.scanNetworks();
  scanCount++;
  totalNetworks += networkCount;

  // Print to serial while scan buffer is still valid
  Serial.print("Scan #"); Serial.println(scanCount);
  Serial.print("Networks found: "); Serial.println(networkCount);
  Serial.print("HDOP: ");
  Serial.print(gps.hdop.hdop(), 2);
  Serial.print(" ("); Serial.print(hdopQuality(gps.hdop.hdop())); Serial.println(")");

  for (int i = 0; i < networkCount; i++) {
    Serial.print("  ["); Serial.print(i + 1); Serial.print("]");
    Serial.print(" SSID:");   Serial.print(WiFi.SSID(i));
    Serial.print(" BSSID:");  Serial.print(WiFi.BSSIDstr(i));
    Serial.print(" CH:");     Serial.print(WiFi.channel(i));
    Serial.print(" RSSI:");   Serial.print(WiFi.RSSI(i));
    Serial.println("dBm");
  }

  // Write CSV while scan buffer is still valid
  csvFile = SD.open(csvFilename, FILE_APPEND);
  if (csvFile) {
    for (int i = 0; i < networkCount; i++) {
      writeNetworkData(i);
    }
    csvFile.close();
  } else {
    displayError("CSV Write Error");
  }

  // Free heap allocated by WiFi.scanNetworks()
  WiFi.scanDelete();
}
