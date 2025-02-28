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

// Pin definitions for GPS module
static const int RXPin = 16, TXPin = 17;
static const uint32_t GPSBaud = 9600;  // GPS module baud rate

// OLED display setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SD_CS_PIN 5       // Chip select pin for SD card
#define MIN_SATELLITES 4  // Minimum number of satellites for a valid GPS fix
#define PST_OFFSET -8     // PST is UTC-8 (unused in optimized code but kept for compatibility)

// Objects for GPS, serial communication, display, and SD card
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
File csvFile;

// Global CSV filename
char csvFilename[32];

// Counters for WiFi scans and networks found
int scanCount = 0;
int networkCount = 0;

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin);
  delay(5000); // Allow GPS module to stabilize

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();

  // Initialize SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(F("Card failed, or not present"));
    displayError("SD Card Error");
    return;
  }

  // Initialize CSV file
  if (!initializeCSV()) {
    displayError("CSV Init Error");
    return;
  }

  rtc.begin(DateTime(F(__DATE__), F(__TIME__))); // Start RTC with compile time
  Serial.println(F("GPS and WiFi Scanner"));
}

void loop() {
  // Read all available GPS data
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Validate GPS fix
  if (gps.location.isValid() && gps.hdop.isValid() && gps.date.isValid() && gps.time.isValid() && gps.satellites.value() >= MIN_SATELLITES) {
    // Sync RTC with GPS time
    DateTime gpsTime(gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour(), gps.time.minute(), gps.time.second());
    rtc.adjust(gpsTime);

    displayGPSData();    // Show current data before scanning
    scanWiFiNetworks();  // Scan and log networks
    displayGPSData();    // Update display with new network count
    delay(15000);        // Wait 15 seconds
  } else {
    displaySearching();  // Show searching status
  }
}

// Display GPS data on OLED
void displayGPSData() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("Lat: "));
  display.println(gps.location.lat(), 6);
  display.print(F("Lng: "));
  display.println(gps.location.lng(), 6);
  display.print(F("Alt: "));
  display.println(gps.altitude.meters());
  display.print(F("Sat: "));
  display.println(gps.satellites.value());

  DateTime now = rtc.now();
  char buffer[20]; // Reduced size since we don’t need full buffer
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  display.print(F("Time: "));
  display.println(buffer);

  display.print(F("Nets: "));
  display.println(networkCount);
  display.print(F("Scans: "));
  display.println(scanCount);
  display.display();
}

// Show GPS searching status
void displaySearching() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Searching for GPS..."));
  display.print(F("Satellites: "));
  display.println(gps.satellites.value());
  display.display();
}

// Display error message
void displayError(const char* error) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(error);
  display.display();
}

// Initialize timestamped CSV file
bool initializeCSV() {
  DateTime now = rtc.now();
  snprintf(csvFilename, sizeof(csvFilename), "/wigle_%04d%02d%02d_%02d%02d%02d.csv", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  csvFile = SD.open(csvFilename, FILE_WRITE);
  if (csvFile) {
    csvFile.println(F("BSSID,SSID,Capabilities,First timestamp seen,Channel,Frequency,RSSI,Latitude,Longitude,Altitude,Accuracy,RCOIs,MfgrId,Type"));
    csvFile.close();
    return true;
  }
  return false;
}

// Scan WiFi networks and log to CSV
void scanWiFiNetworks() {
  networkCount = WiFi.scanNetworks();
  scanCount++;
  csvFile = SD.open(csvFilename, FILE_APPEND);
  if (csvFile) {
    // Precompute values constant across all networks in this scan
    const char* timestamp = getLocalTimestamp();
    char latStr[12];
    dtostrf(gps.location.lat(), 11, 6, latStr);
    char lngStr[12];
    dtostrf(gps.location.lng(), 11, 6, lngStr);
    char altStr[10];
    dtostrf(gps.altitude.meters(), 9, 2, altStr);
    char accStr[10];
    dtostrf(gps.hdop.hdop(), 9, 2, accStr);

    for (int i = 0; i < networkCount; ++i) {
      uint8_t* bssid = WiFi.BSSID(i);
      const char* ssid = WiFi.SSID(i).c_str();
      const char* capabilities = WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" : "Secured";
      int channel = WiFi.channel(i);
      int frequency = 2400 + (channel - 1) * 5; // 2.4GHz approximation
      int rssi = WiFi.RSSI(i);

      // Format entire CSV line in one buffer
      char line[512];
      snprintf(line, sizeof(line),
        "\"%02X:%02X:%02X:%02X:%02X:%02X\",\"%s\",\"%s\",\"%s\",%d,%d,%d,%s,%s,%s,%s,\"\",\"\",\"WiFi\"",
        bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
        ssid,
        capabilities,
        timestamp,
        channel,
        frequency,
        rssi,
        latStr,
        lngStr,
        altStr,
        accStr
      );
      csvFile.println(line);
    }
    csvFile.close();
  } else {
    displayError("CSV Write Error");
  }

  // Serial monitor output
  Serial.print(F("Scan #"));
  Serial.println(scanCount);
  Serial.print(F("Networks found: "));
  Serial.println(networkCount);
  for (int i = 0; i < networkCount; ++i) {
    Serial.print(F("Network #"));
    Serial.print(i + 1);
    Serial.print(F(": "));
    Serial.print(WiFi.SSID(i));
    Serial.print(F(" ("));
    Serial.print(WiFi.RSSI(i));
    Serial.println(F(" dBm)"));
  }
}

// Get current timestamp as const char*
const char* getLocalTimestamp() {
  static char buffer[20]; // Reduced size for efficiency
  DateTime now = rtc.now();
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  return buffer;
}
