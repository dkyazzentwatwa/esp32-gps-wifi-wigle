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
#define PST_OFFSET -8     // PST is UTC-8

// Objects for GPS, serial communication, display, and SD card
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
File csvFile;

// Counters for WiFi scans and networks found
int scanCount = 0;
int networkCount = 0;

void setup() {
  Serial.begin(115200); // Start serial communication for debugging
  gpsSerial.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin); // Start GPS serial communication
  delay(5000); // Allow time for the system to stabilize

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Infinite loop if display initialization fails
  }
  display.display();
  delay(2000);
  display.clearDisplay();

  // Initialize SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Card failed, or not present");
    displayError("SD Card Error");
    return;
  }

  // Initialize CSV file
  if (!initializeCSV()) {
    displayError("CSV Init Error");
    return;
  }

  rtc.begin(DateTime(F(__DATE__), F(__TIME__))); // Initialize RTC with compile time

  Serial.println(F("GPS and WiFi Scanner"));
}

void loop() {
  // Read GPS data
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Check if GPS data is valid
  if (gps.location.isValid() && gps.hdop.isValid() && gps.date.isValid() && gps.time.isValid() && gps.satellites.value() >= MIN_SATELLITES) {
    // Update RTC with GPS time
    DateTime gpsTime(gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour(), gps.time.minute(), gps.time.second());
    rtc.adjust(gpsTime);

    // Display GPS data and scan for WiFi networks
    displayGPSData();
    scanWiFiNetworks();
    displayGPSData();
    delay(15000); // Wait 15 seconds before next scan
  } else {
    displaySearching();
  }
}

// Display GPS data on the OLED screen
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
  display.print("Sat: ");
  display.println(gps.satellites.value());

  // Get the current time from RTC and display it
  DateTime now = rtc.now();
  char buffer[30];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  display.print("Time: ");
  display.println(buffer);

  // Display the number of networks found and scan count
  display.print("Nets: ");
  display.println(networkCount);
  display.print("Scans: ");
  display.println(scanCount);

  display.display();
}

// Display a message while searching for GPS signal
void displaySearching() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Searching for GPS...");
  display.print("Satellites: ");
  display.println(gps.satellites.value());
  display.display();
}

// Display an error message on the OLED screen
void displayError(const char* error) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(error);
  display.display();
}

// Initialize a new CSV file for storing WiFi scan results
bool initializeCSV() {
  DateTime now = rtc.now();
  char filename[32];
  snprintf(filename, sizeof(filename), "/wigle_%04d%02d%02d_%02d%02d%02d.csv", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  csvFile = SD.open(filename, FILE_WRITE);
  if (csvFile) {
    csvFile.println("BSSID,SSID,Capabilities,First timestamp seen,Channel,Frequency,RSSI,Latitude,Longitude,Altitude,Accuracy,RCOIs,MfgrId,Type");
    csvFile.close();
    return true;
  }
  return false;
}

// Scan for WiFi networks and store results in the CSV file
void scanWiFiNetworks() {
  networkCount = WiFi.scanNetworks(); // Scan for WiFi networks
  scanCount++; // Increment scan count
  csvFile = SD.open("/wigle.csv", FILE_APPEND);
  if (csvFile) {
    for (int i = 0; i < networkCount; ++i) {
      writeNetworkData(i);
    }
    csvFile.close();
  } else {
    displayError("CSV Write Error");
  }

  // Print networks to serial monitor
  Serial.print("Scan #");
  Serial.println(scanCount);
  Serial.print("Networks found: ");
  Serial.println(networkCount);
  for (int i = 0; i < networkCount; ++i) {
    Serial.print("Network #");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" (");
    Serial.print(WiFi.RSSI(i));
    Serial.println(" dBm)");
  }
}

// Write network data to the CSV file
void writeNetworkData(int networkIndex) {
  String bssid = WiFi.BSSIDstr(networkIndex);
  String ssid = WiFi.SSID(networkIndex);
  String capabilities = WiFi.encryptionType(networkIndex) == WIFI_AUTH_OPEN ? "Open" : "Secured";
  String timestamp = getLocalTimestamp();
  int channel = WiFi.channel(networkIndex);
  int frequency = 2400 + (channel - 1) * 5;  // Assuming 2.4GHz WiFi
  int rssi = WiFi.RSSI(networkIndex);
  String latitude = String(gps.location.lat(), 6);
  String longitude = String(gps.location.lng(), 6);
  String altitude = String(gps.altitude.meters());
  String accuracy = String(gps.hdop.hdop());
  String rcois = "";   // Placeholder if RCOIs is not available
  String mfgrid = "";  // Placeholder if Manufacturer ID is not available
  String type = "WiFi";

  // Write network data to CSV file
  csvFile.print("\"" + bssid + "\",");
  csvFile.print("\"" + ssid + "\",");
  csvFile.print("\"" + capabilities + "\",");
  csvFile.print("\"" + timestamp + "\",");
  csvFile.print(channel);
  csvFile.print(",");
  csvFile.print(frequency);
  csvFile.print(",");
  csvFile.print(rssi);
  csvFile.print(",");
  csvFile.print(latitude);
  csvFile.print(",");
  csvFile.print(longitude);
  csvFile.print(",");
  csvFile.print(altitude);
  csvFile.print(",");
  csvFile.print(accuracy);
  csvFile.print(",");
  csvFile.print("\"" + rcois + "\",");
  csvFile.print("\"" + mfgrid + "\",");
  csvFile.println("\"" + type + "\"");
}

// Get the current timestamp from the RTC
String getLocalTimestamp() {
  DateTime now = rtc.now();  // Use RTC time for timestamp
  char buffer[30];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  return String(buffer);
}