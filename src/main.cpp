#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <EEPROM.h>
#include "ble_viewer.h"
#include "wifi_viewer.h"

// WiFi credentials - Can be updated via Serial or Access Point
String ssid = "YOUR_SSID_HERE"; // Set your WiFi SSID here
String password = "YOUR_PASSWORD_HERE"; // Set your WiFi password here

// EEPROM settings for WiFi credentials
#define EEPROM_SIZE 128
#define SSID_ADDR 0
#define SSID_LEN  64
#define PASS_ADDR 64
#define PASS_LEN  64

// Display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Configurable display text strings - easy to customize
const char* SPLASH_TITLE = "MERCALLI SEISMO";
const char* SPLASH_COPYRIGHT = "(c) 2025 JOHN SCHOP";
const char* RESET_MESSAGE = "RESET";
const char* PEAK_VALUES_HEADER = "PEAK (m/s^2):";
const char* BASELINE_HEADER = "ESTABLISHING BASELINE";
const char* MERCALLI_LABEL = "MERCALLI";
const char* NOW_LABEL = "Now: ";
const char* BASELINE_STATUS = "Baseline";
const char* SETUP_STATUS = "Setup";
const char* CALIBRATING_MESSAGE = "CALIBRATING...";
const char* KEEP_STILL_MESSAGE = "Keep device STILL";
const char* CALIBRATION_HEADER = "CALIBRATION";
const char* COMPLETE_MESSAGE = "COMPLETE";
const char* WARNING_MESSAGE = "WARNING!";
const char* CALIBRATION_ISSUE = "Calibration issue";
const char* FAILED_MESSAGE = "FAILED!";
const char* ADXL345_ERROR = "ADXL345 ERROR!";

// Axis and formatting labels
const char* X_LABEL = "X: ";
const char* Y_LABEL = "Y: ";
const char* Z_LABEL = "Z: ";
const char* PROGRESS_LABEL = "Progress: ";
const char* TIME_LEFT_LABEL = "Time left: ";
const char* NOISE_LABEL = "Noise: ";

// Mercalli intensity thresholds (m/s²) - easy to adjust for sensor sensitivity
const float MERCALLI_1_THRESHOLD = 0.15;  // I - Not felt (accounts for sensor noise)
const float MERCALLI_2_THRESHOLD = 0.25;  // II - Weak
const float MERCALLI_3_THRESHOLD = 0.4;   // III - Weak
const float MERCALLI_4_THRESHOLD = 0.7;   // IV - Light
const float MERCALLI_5_THRESHOLD = 1.2;   // V - Moderate
const float MERCALLI_6_THRESHOLD = 2.0;   // VI - Strong
const float MERCALLI_7_THRESHOLD = 4.0;   // VII - Very strong
const float MERCALLI_8_THRESHOLD = 8.0;   // VIII - Severe
const float MERCALLI_9_THRESHOLD = 12.0;  // IX - Violent
const float MERCALLI_10_THRESHOLD = 16.0; // X - Extreme
const float MERCALLI_11_THRESHOLD = 20.0; // XI - Extreme
// XII - Extreme (anything above MERCALLI_11_THRESHOLD)

// Web server
WebServer server(80);

// BLE Server
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define DATA_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define RESET_CHARACTERISTIC_UUID "ec0e0001-36e1-4688-b7f5-ea07361b26a8"
BLEServer* pServer = NULL;
BLECharacteristic* pDataCharacteristic = NULL;
bool deviceConnected = false;

// Create display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Create ADXL345 object
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Variables for seismometer data
float x_accel, y_accel, z_accel;
float magnitude;
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 100; // Update every 100ms

// Peak value tracking (deviation from baseline)
float x_peak = 0, y_peak = 0, z_peak = 0;
float magnitude_peak = 0;
float deviation_magnitude_peak = 0; // Track peak deviation magnitude separately
int mercalli_peak = 0;
bool resetRequested = false;

// Moving baseline for detecting changes
float x_baseline = 0, y_baseline = 0, z_baseline = 0;
const float baseline_alpha = 0.95; // Smoothing factor for baseline (higher = slower adaptation)
const int baseline_samples = 20; // Number of samples before starting peak detection
int sample_count = 0;

// Noise filtering
float noise_threshold = 0.1; // Will be automatically determined during calibration

// Software calibration offsets (applied in software, not hardware)
float calibration_offset_x = 0;
float calibration_offset_y = 0;
float calibration_offset_z = 0;
bool calibrated = false;

// Button pin for reset (optional - can use serial command instead)
#define RESET_BUTTON_PIN 4  // Changed to GPIO4; GPIO2 can be problematic on some boards.

// Function declarations
void updateDisplay();
void resetPeakValues();
void checkForSerialCommand();
void calibrateAccelerometer();
int calculateMercalli(float magnitude);
void setupWifi();
void setupBLE();
void handleRoot();
void handleData();
void handleReset();
void handleBleViewer();
void handleWifiConfig();
void handleWifiSave();
String getSensorDataJson();
void saveWifiCredentials();
void loadWifiCredentials();
void printStatus();

// BLE Callback Classes
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("BLE Client Connected");
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("BLE Client Disconnected");
      // Restart advertising so a new client can connect
      BLEDevice::startAdvertising();
    }
};

class ResetCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            Serial.println("BLE: Reset command received");
            resetPeakValues();
        }
    }
};

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  loadWifiCredentials();
  
  // Initialize I2C
  Wire.begin();
  
  // Initialize reset button (optional)
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  
  // Initialize the display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  // Initialize the accelerometer
  if(!accel.begin()) {
    Serial.println("No ADXL345 detected");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println(ADXL345_ERROR);
    display.display();
    for(;;); // Don't proceed, loop forever
  }
  
  // Set accelerometer range (optional)
  accel.setRange(ADXL345_RANGE_4_G);
  
  // Show splash screen
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  int16_t x1, y1;
  uint16_t text_width, text_height;
  
  // Center splash title (large text)
  display.setTextSize(1);
  display.getTextBounds(SPLASH_TITLE, 0, 0, &x1, &y1, &text_width, &text_height);
  display.setCursor((SCREEN_WIDTH - text_width) / 2, 15);
  display.println(SPLASH_TITLE);
  
  // Center copyright (small text)
  display.setTextSize(1);
  display.getTextBounds(SPLASH_COPYRIGHT, 0, 0, &x1, &y1, &text_width, &text_height);
  display.setCursor((SCREEN_WIDTH - text_width) / 2, 45);
  display.println(SPLASH_COPYRIGHT);
  
  display.display();

  delay(2000);
  
  // Setup WiFi and display IP
  setupWifi();
  
  // Start web server if WiFi is connected OR in AP mode
  if (WiFi.status() == WL_CONNECTED || WiFi.getMode() == WIFI_AP) {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/data", HTTP_GET, handleData);
    server.on("/reset", HTTP_POST, handleReset);
    server.on("/ble", HTTP_GET, handleBleViewer);
    server.on("/config", HTTP_GET, handleWifiConfig);
    server.on("/save", HTTP_POST, handleWifiSave);
    server.begin();
    Serial.println("Web server started");
  }

  // Setup BLE
  setupBLE();

  // Calibrate the accelerometer
  calibrateAccelerometer();
  
  // Reset peak values after calibration to start fresh
  resetPeakValues();
  
  
  Serial.println(F("Seismometer initialized successfully."));
  Serial.println(F("Available serial commands: STATUS, RESET, CALIBRATE, SSID <name>, PASS <password>, BOOT"));
  Serial.println(F("Press button on GPIO 4 to reset peak values."));
  
  if (WiFi.getMode() == WIFI_AP) {
    Serial.println(F("*** Access Point Mode Active ***"));
    Serial.println(F("Connect to the AP and go to http://192.168.4.1 to configure WiFi"));
  }
}

void loop() {
  // Handle web server requests
  if (WiFi.status() == WL_CONNECTED || WiFi.getMode() == WIFI_AP) {
    server.handleClient();
  }

  // Check for reset command
  checkForSerialCommand();
  
  if (millis() - lastUpdate >= updateInterval) {
    // Get a new sensor reading
    sensors_event_t event;
    accel.getEvent(&event);
    
    // Store accelerometer values and apply software calibration
    x_accel = event.acceleration.x + calibration_offset_x;
    y_accel = event.acceleration.y + calibration_offset_y;
    z_accel = event.acceleration.z + calibration_offset_z;
    
    // Calculate magnitude of acceleration vector
    magnitude = sqrt(x_accel*x_accel + y_accel*y_accel + z_accel*z_accel);
    
    // Update moving baseline (exponential moving average)
    if (sample_count < baseline_samples) {
      // Initial baseline establishment
      x_baseline = x_accel;
      y_baseline = y_accel;
      z_baseline = z_accel;
      sample_count++;
    } else {
      // Slowly adapt baseline to current position
      x_baseline = baseline_alpha * x_baseline + (1.0 - baseline_alpha) * x_accel;
      y_baseline = baseline_alpha * y_baseline + (1.0 - baseline_alpha) * y_accel;
      z_baseline = baseline_alpha * z_baseline + (1.0 - baseline_alpha) * z_accel;
      
      // Calculate deviations from baseline
      float x_deviation = abs(x_accel - x_baseline);
      float y_deviation = abs(y_accel - y_baseline);
      float z_deviation = abs(z_accel - z_baseline);
      
      // Apply noise threshold - ignore small deviations
      if (x_deviation < noise_threshold) x_deviation = 0;
      if (y_deviation < noise_threshold) y_deviation = 0;
      if (z_deviation < noise_threshold) z_deviation = 0;
      
      // Calculate deviation magnitude
      float deviation_magnitude = sqrt(x_deviation*x_deviation + y_deviation*y_deviation + z_deviation*z_deviation);
      
      // Update peak deviations
      if (x_deviation > x_peak) x_peak = x_deviation;
      if (y_deviation > y_peak) y_peak = y_deviation;
      if (z_deviation > z_peak) z_peak = z_deviation;
      
      // Update peak deviation magnitude and Mercalli (based on deviation, not raw magnitude)
      if (deviation_magnitude > deviation_magnitude_peak) {
        deviation_magnitude_peak = deviation_magnitude;
        mercalli_peak = calculateMercalli(deviation_magnitude_peak);
      }
      
      // Still track raw magnitude peak for reference
      if (magnitude > magnitude_peak) {
        magnitude_peak = magnitude;
      }
    }
    
    // Update display
    updateDisplay();

    // Notify BLE client if connected
    if (deviceConnected) {
      String jsonData = getSensorDataJson();
      pDataCharacteristic->setValue(jsonData.c_str());
      pDataCharacteristic->notify();
    }
    
    lastUpdate = millis();
  }
}

void updateDisplay() {
  display.clearDisplay();
  
  // Display header
  display.setTextSize(1);
  display.setCursor(0, 2);
  if (sample_count >= baseline_samples) {
    display.println(PEAK_VALUES_HEADER);
  } else {
    display.println(BASELINE_HEADER);
  }
  
  // Draw separator line
  display.drawLine(0, 12, SCREEN_WIDTH, 12, SSD1306_WHITE);
  
  // Display peak values with better spacing and smaller font
  display.setTextSize(1);
  display.setCursor(0, 15);
  display.print(X_LABEL);
  display.setTextSize(2);
  display.print(x_peak, 2);
  
  display.setTextSize(1);
  display.setCursor(0, 32);
  display.print(Y_LABEL);
  display.setTextSize(2);
  display.print(y_peak, 2);
  
  display.setTextSize(1);
  display.setCursor(0, 49);
  display.print(Z_LABEL);
  display.setTextSize(2);
  display.print(z_peak, 2);
  
  // Display Mercalli intensity on the right side with better layout
  if (sample_count >= baseline_samples) {
    // Calculate current deviation magnitude for Mercalli display
    float x_deviation = abs(x_accel - x_baseline);
    float y_deviation = abs(y_accel - y_baseline);
    float z_deviation = abs(z_accel - z_baseline);
    
    // Apply noise threshold - ignore small deviations
    if (x_deviation < noise_threshold) x_deviation = 0;
    if (y_deviation < noise_threshold) y_deviation = 0;
    if (z_deviation < noise_threshold) z_deviation = 0;
    
    float deviation_magnitude = sqrt(x_deviation*x_deviation + y_deviation*y_deviation + z_deviation*z_deviation);
    int current_mercalli = calculateMercalli(deviation_magnitude);
    
    display.setTextSize(1);
    display.setCursor(80, 15);
    display.println(MERCALLI_LABEL);
    
    // Peak Mercalli in large font (most important)
    display.setTextSize(3);
    display.setCursor(90, 25);
    display.print(mercalli_peak);
    
    // Current Mercalli below in smaller font
    display.setTextSize(1);
    display.setCursor(80, 50);
    display.print(NOW_LABEL);
    display.setTextSize(1);
    display.print(current_mercalli);
  } else {
    display.setTextSize(1);
    display.setCursor(80, 25);
    display.println(BASELINE_STATUS);
    display.setCursor(80, 35);
    display.println(SETUP_STATUS);
    display.setCursor(80, 45);
    display.print(sample_count);
    display.print("/");
    display.print(baseline_samples);
  }
  
  display.display();
}

void resetPeakValues() {
  x_peak = 0;
  y_peak = 0;
  z_peak = 0;
  magnitude_peak = 0;
  deviation_magnitude_peak = 0;
  mercalli_peak = 0;
  sample_count = 0; // Reset baseline establishment
  Serial.println("Peak values and baseline reset.");
  
  // Show reset confirmation on display briefly
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  
  // Center reset message dynamically
  int16_t x1, y1;
  uint16_t text_width, text_height;
  display.getTextBounds(RESET_MESSAGE, 0, 0, &x1, &y1, &text_width, &text_height);
  display.setCursor((SCREEN_WIDTH - text_width) / 2, 20);
  display.println(RESET_MESSAGE);
  
  display.display();
  delay(400);
}

void checkForSerialCommand() {
  // Check for serial command
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    String upperCommand = command;
    upperCommand.toUpperCase();

    if (upperCommand == "RESET") {
      resetPeakValues();
    } else if (upperCommand == "CALIBRATE") {
      calibrateAccelerometer();
    } else if (upperCommand == "BOOT") {
      ESP.restart();
    } else if (upperCommand == "STATUS") {
      Serial.println(F("--- System Status ---"));
      
      // WiFi Status
      Serial.print(F("WiFi Status: "));
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println(F("Connected"));
        Serial.print(F("  SSID: "));
        Serial.println(ssid);
        Serial.print(F("  IP Address: "));
        Serial.println(WiFi.localIP());
      } else if (WiFi.getMode() == WIFI_AP) {
        Serial.println(F("Access Point Mode"));
        Serial.print(F("  AP Name: Seismometer-"));
        String mac = WiFi.macAddress();
        mac.replace(":", "");
        Serial.println(mac.substring(6));
        Serial.print(F("  AP IP: "));
        Serial.println(WiFi.softAPIP());
      } else {
        Serial.println(F("Offline"));
        Serial.print(F("  Configured SSID: "));
        Serial.println(ssid);
      }
      Serial.print(F("  Stored Password: "));
      if (password.length() > 0) {
          Serial.println(password);
      } else {
          Serial.println(F("[not set]"));
      }

      // BLE Status
      Serial.print(F("BLE Status: "));
      if (deviceConnected) {
        Serial.println(F("Client Connected"));
      } else {
        Serial.println(F("Advertising"));
      }

      // Calibration Status
      Serial.print(F("Calibration Status: "));
      if (calibrated) {
        Serial.println(F("Complete"));
        Serial.print(F("  Noise Threshold: "));
        Serial.println(noise_threshold, 4);
        Serial.print(F("  Offsets (X,Y,Z): "));
        Serial.print(calibration_offset_x, 3);
        Serial.print(F(", "));
        Serial.print(calibration_offset_y, 3);
        Serial.print(F(", "));
        Serial.println(calibration_offset_z, 3);
      } else {
        Serial.println(F("Not Calibrated"));
      }
      Serial.println(F("---------------------"));
    } else if (upperCommand.startsWith("SSID ")) {
      String newSsid = command.substring(5);
      ssid = newSsid;
      saveWifiCredentials();
      Serial.println("SSID updated to: " + newSsid);
      Serial.println("Rebooting to apply changes...");
      delay(1000);
      ESP.restart();
    } else if (upperCommand.startsWith("PASS ")) {
      String newPass = command.substring(5);
      password = newPass;
      saveWifiCredentials();
      Serial.println("Password updated.");
      Serial.println("Rebooting to apply changes...");
      delay(1000);
      ESP.restart();
    } else {
      Serial.print(F("Unknown command: "));
      Serial.println(command);
    }
  }
  
  // Check for button press with debouncing
  static int buttonState = HIGH;
  static int lastButtonState = HIGH;
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 50;

  int reading = digitalRead(RESET_BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        Serial.println("Button was pressed. Resetting values.");
        resetPeakValues();
      } else { // buttonState is HIGH
        Serial.println("Button was released.");
      }
    }
  }
  
  lastButtonState = reading;
}

void calibrateAccelerometer() {
  Serial.println("Starting accelerometer calibration...");
  Serial.println("Keep the device still during calibration.");
  
  // Display calibration message
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println(CALIBRATING_MESSAGE);
  display.setCursor(0, 25);
  display.println(KEEP_STILL_MESSAGE);
  display.println(WiFi.localIP().toString());
  display.display();
  
  // Wait a bit for user to read message
  delay(2000);
  
  // Clear any existing hardware offsets first
  Wire.beginTransmission(0x53);
  Wire.write(0x1E); Wire.write(0); // Clear OFSX
  Wire.endTransmission();
  Wire.beginTransmission(0x53);
  Wire.write(0x1F); Wire.write(0); // Clear OFSY
  Wire.endTransmission();
  Wire.beginTransmission(0x53);
  Wire.write(0x20); Wire.write(0); // Clear OFSZ
  Wire.endTransmission();
  delay(100);
  
  // Calibration parameters
  const int numSamples = 100;
  const int sampleDelay = 50; // 50ms between samples
  float sumX = 0, sumY = 0, sumZ = 0;
  float sumX2 = 0, sumY2 = 0, sumZ2 = 0; // For standard deviation calculation
  int validSamples = 0;
  
  // Show countdown and collect samples
  for (int i = 0; i < numSamples; i++) {
    // Update display with progress
    if (i % 10 == 0) { // Update every 10 samples
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 10);
      display.println(CALIBRATING_MESSAGE);
      display.setCursor(0, 25);
      display.print(PROGRESS_LABEL);
      display.print((i * 100) / numSamples);
      display.println("%");
      
      // Countdown timer
      int secondsLeft = ((numSamples - i) * sampleDelay) / 1000;
      display.setCursor(0, 40);
      display.print(TIME_LEFT_LABEL);
      display.print(secondsLeft);
      display.println("s");

      // ip address
      display.setCursor(0, 55);
      display.print("IP: ");
      display.print(WiFi.localIP().toString());
      display.display();
    }
    
    // Get sensor reading (raw, no calibration applied yet)
    sensors_event_t event;
    if (accel.getEvent(&event)) {
      sumX += event.acceleration.x;
      sumY += event.acceleration.y;
      sumZ += event.acceleration.z;
      
      // Collect sum of squares for standard deviation calculation
      sumX2 += event.acceleration.x * event.acceleration.x;
      sumY2 += event.acceleration.y * event.acceleration.y;
      sumZ2 += event.acceleration.z * event.acceleration.z;
      
      validSamples++;
    }
    
    delay(sampleDelay);
  }
  
  if (validSamples > 0) {
    // Calculate average raw readings
    float avgX = sumX / validSamples;
    float avgY = sumY / validSamples;
    float avgZ = sumZ / validSamples;
    
    // Calculate standard deviation for noise threshold determination
    float varX = (sumX2 / validSamples) - (avgX * avgX);
    float varY = (sumY2 / validSamples) - (avgY * avgY);
    float varZ = (sumZ2 / validSamples) - (avgZ * avgZ);
    
    float stdX = sqrt(varX);
    float stdY = sqrt(varY);
    float stdZ = sqrt(varZ);
    
    // Use 3 times the maximum standard deviation as noise threshold
    // This should capture 99.7% of noise variations (3-sigma rule)
    float maxStd = max(max(stdX, stdY), stdZ);
    noise_threshold = 3.0 * maxStd;
    
    // Ensure minimum threshold of 0.05 m/s2 for very quiet sensors
    if (noise_threshold < 0.05) noise_threshold = 0.05;
    
    Serial.print("Noise analysis - StdDev X: "); Serial.print(stdX, 4);
    Serial.print(" Y: "); Serial.print(stdY, 4);
    Serial.print(" Z: "); Serial.println(stdZ, 4);
    Serial.print("Auto noise threshold set to: "); Serial.print(noise_threshold, 4);
    Serial.println(" m/s2");
    
    // Calculate software calibration offsets
    // These will be applied in software to make all axes read ~0
    calibration_offset_x = -avgX;
    calibration_offset_y = -avgY;
    calibration_offset_z = -avgZ;
    calibrated = true;
    
    Serial.println("Software calibration complete.");
    Serial.print("Raw averages - X: "); Serial.print(avgX, 3);
    Serial.print(" Y: "); Serial.print(avgY, 3);
    Serial.print(" Z: "); Serial.println(avgZ, 3);
    Serial.print("Software offsets - X: "); Serial.print(calibration_offset_x, 3);
    Serial.print(" Y: "); Serial.print(calibration_offset_y, 3);
    Serial.print(" Z: "); Serial.println(calibration_offset_z, 3);
    
    // Verify calibration by taking test readings with software offsets applied
    Serial.println("Taking verification readings...");
    float testSumX = 0, testSumY = 0, testSumZ = 0;
    int testSamples = 10;
    
    for (int i = 0; i < testSamples; i++) {
      sensors_event_t event;
      if (accel.getEvent(&event)) {
        float calibratedX = event.acceleration.x + calibration_offset_x;
        float calibratedY = event.acceleration.y + calibration_offset_y;
        float calibratedZ = event.acceleration.z + calibration_offset_z;
        
        testSumX += calibratedX;
        testSumY += calibratedY;
        testSumZ += calibratedZ;
        
        Serial.print("Sample "); Serial.print(i+1); Serial.print(": ");
        Serial.print("X:"); Serial.print(calibratedX, 3);
        Serial.print(" Y:"); Serial.print(calibratedY, 3);
        Serial.print(" Z:"); Serial.println(calibratedZ, 3);
      }
      delay(50);
    }
    
    float testAvgX = testSumX / testSamples;
    float testAvgY = testSumY / testSamples;
    float testAvgZ = testSumZ / testSamples;
    
    Serial.print("Calibrated averages - X: "); Serial.print(testAvgX, 3);
    Serial.print(" Y: "); Serial.print(testAvgY, 3);
    Serial.print(" Z: "); Serial.println(testAvgZ, 3);
    
    // Check if calibration was successful
    bool calibrationGood = (abs(testAvgX) < 0.1 && abs(testAvgY) < 0.1 && abs(testAvgZ) < 0.1);
    
    // Show completion on display
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 5);
    display.println(CALIBRATION_HEADER);
    if (calibrationGood) {
      display.setCursor(0, 20);
      display.println(COMPLETE_MESSAGE);
      display.setCursor(0, 30);
      display.print(X_LABEL);
      display.print(testAvgX, 2);
      display.print(Y_LABEL);
      display.print(testAvgY, 2);
      display.setCursor(0, 40);
      display.print(Z_LABEL);
      display.print(testAvgZ, 2);
      display.println(" m/s2");
      display.setCursor(0, 50);
      display.print(NOISE_LABEL);
      display.print(noise_threshold, 3);
      Serial.println("Software calibration successful.");
    } else {
      display.setCursor(0, 20);
      display.println(WARNING_MESSAGE);
      display.setCursor(0, 35);
      display.println(CALIBRATION_ISSUE);
      Serial.println("Software calibration may have issues.");
    }

    display.display();
    delay(3000);
    
  } else {
    Serial.println("Calibration failed - no valid samples.");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 20);
    display.println(CALIBRATION_HEADER);
    display.setCursor(0, 35);
    display.println(FAILED_MESSAGE);
    display.display();
    delay(3000);
  }
}

int calculateMercalli(float magnitude) {
  // Simple linear mapping from magnitude to Mercalli intensity
  // Adjust the mapping as needed for your specific requirements
  if (magnitude < MERCALLI_1_THRESHOLD) return 1;
  else if (magnitude < MERCALLI_2_THRESHOLD) return 2;
  else if (magnitude < MERCALLI_3_THRESHOLD) return 3;
  else if (magnitude < MERCALLI_4_THRESHOLD) return 4;
  else if (magnitude < MERCALLI_5_THRESHOLD) return 5;
  else if (magnitude < MERCALLI_6_THRESHOLD) return 6;
  else if (magnitude < MERCALLI_7_THRESHOLD) return 7;
  else if (magnitude < MERCALLI_8_THRESHOLD) return 8;
  else if (magnitude < MERCALLI_9_THRESHOLD) return 9;
  else if (magnitude < MERCALLI_10_THRESHOLD) return 10;
  else if (magnitude < MERCALLI_11_THRESHOLD) return 11;
  else return 12; // XII - Extreme
}

void setupWifi() {
  delay(10);
  Serial.print(F("Connecting to WiFi"));

  // Attempt to connect to WiFi network
  WiFi.begin(ssid.c_str(), password.c_str());
  
  // Wait for connection
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("Connected to WiFi"));
    Serial.print(F("IP Address: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("Failed to connect to WiFi. Starting Access Point mode..."));
    
    // Start Access Point
    WiFi.mode(WIFI_AP);
    String apName = "Seismometer";
    apName.replace(":", "");
    
    if (WiFi.softAP(apName.c_str())) {
      Serial.println(F("Access Point started"));
      Serial.print(F("AP Name: "));
      Serial.println(apName);
      Serial.print(F("AP IP Address: "));
      Serial.println(WiFi.softAPIP());
      Serial.println(F("Connect to this AP and go to http://192.168.4.1 to configure WiFi"));
      
      // Display AP info on OLED
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 5);
      display.println("WiFi Setup Mode");
      display.setCursor(0, 20);
      display.println("Connect to:");
      display.setCursor(0, 35);
      display.println(apName);
      display.setCursor(0, 50);
      display.println("192.168.4.1");
      display.display();
      delay(3000);
    } else {
      Serial.println(F("Failed to start Access Point"));
    }
  }
}

void setupBLE() {
  // Create the BLE Device
  BLEDevice::init("Seismometer");
  
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // Create the BLE Characteristics
  pDataCharacteristic = pService->createCharacteristic(
                        DATA_CHARACTERISTIC_UUID,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  
  BLECharacteristic *pResetCharacteristic = pService->createCharacteristic(
                        RESET_CHARACTERISTIC_UUID,
                        BLECharacteristic::PROPERTY_WRITE
                      );
  
  // Set the reset characteristic callback
  pResetCharacteristic->setCallbacks(new ResetCharacteristicCallbacks());
  
  // Start the service
  pService->start();
  
  // Start advertising
  BLEDevice::startAdvertising();
  
  Serial.println(F("BLE Server setup complete, advertising..."));
}

void handleRoot() {
  // If in AP mode, show WiFi config page, otherwise show normal dashboard
  if (WiFi.getMode() == WIFI_AP) {
    handleWifiConfig();
  } else {
    String page = FPSTR(WIFI_HTML_PAGE);
    page.replace("%IP_ADDRESS%", WiFi.localIP().toString());
    server.send(200, "text/html", page);
  }
}

void handleData() {
  String json = getSensorDataJson();
  server.send(200, "application/json", json);
}

void handleBleViewer() {
  server.send(200, "text/html", HTML_PAGE);
}

void handleWifiConfig() {
  String html = "<!DOCTYPE html><html><head><title>Seismometer WiFi Setup</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0}";
  html += ".container{max-width:400px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}";
  html += "h1{color:#333;text-align:center;margin-bottom:30px}";
  html += ".form-group{margin-bottom:20px}";
  html += "label{display:block;margin-bottom:5px;font-weight:bold;color:#555}";
  html += "input[type='text'],input[type='password']{width:100%;padding:10px;border:1px solid #ddd;border-radius:5px;font-size:16px;box-sizing:border-box}";
  html += "button{width:100%;padding:12px;background:#007bff;color:white;border:none;border-radius:5px;font-size:16px;cursor:pointer}";
  html += "button:hover{background:#0056b3}";
  html += ".status{text-align:center;margin-top:20px;padding:10px;background:#e7f3ff;border-radius:5px}";
  html += ".sensor-data{margin-top:20px;padding:15px;background:#f8f9fa;border-radius:5px}";
  html += ".sensor-data h3{margin-top:0;color:#333}";
  html += ".mercalli{font-size:24px;font-weight:bold;color:#dc3545}";
  html += ".refresh-btn{margin-top:10px;padding:8px 16px;background:#28a745;color:white;border:none;border-radius:5px;cursor:pointer}";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>🌍 Seismometer WiFi Setup</h1>";
  html += "<form action='/save' method='POST'>";
  html += "<div class='form-group'>";
  html += "<label for='ssid'>WiFi Network Name (SSID):</label>";
  html += "<input type='text' id='ssid' name='ssid' required>";
  html += "</div>";
  html += "<div class='form-group'>";
  html += "<label for='password'>WiFi Password:</label>";
  html += "<input type='password' id='password' name='password'>";
  html += "</div>";
  html += "<button type='submit'>Save & Connect</button>";
  html += "</form>";
  html += "<div class='status'>";
  html += "<p><strong>Current Status:</strong> Access Point Mode</p>";
  html += "<p>Device continues monitoring seismic activity</p>";
  html += "</div>";
  html += "<div class='sensor-data'>";
  html += "<h3>Live Seismic Data</h3>";
  html += "<div id='sensorInfo'>Loading...</div>";
  html += "<button class='refresh-btn' onclick='updateSensorData()'>Refresh Data</button>";
  html += "</div>";
  html += "</div>";
  html += "<script>";
  html += "function updateSensorData(){";
  html += "fetch('/data').then(response=>response.json()).then(data=>{";
  html += "document.getElementById('sensorInfo').innerHTML=";
  html += "'<div class=\"mercalli\">Mercalli Peak: '+data.mercalli_peak+'</div>';";
  html += "document.getElementById('sensorInfo').innerHTML+=";
  html += "'<div>Current: '+data.mercalli_now+'</div>';";
  html += "document.getElementById('sensorInfo').innerHTML+=";
  html += "'<div>Peak Deviations - X: '+data.x_peak.toFixed(3)+', Y: '+data.y_peak.toFixed(3)+', Z: '+data.z_peak.toFixed(3)+'</div>';";
  html += "}).catch(error=>{";
  html += "document.getElementById('sensorInfo').innerHTML='Error loading sensor data';";
  html += "});}";
  html += "setInterval(updateSensorData,5000);";
  html += "updateSensorData();";
  html += "</script>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleWifiSave() {
  if (server.hasArg("ssid")) {
    String newSsid = server.arg("ssid");
    String newPass = server.hasArg("password") ? server.arg("password") : "";
    
    Serial.println("WiFi credentials received via web interface:");
    Serial.println("SSID: " + newSsid);
    if (newPass.length() > 0) {
      Serial.println("Password: [set]");
    } else {
      Serial.println("Password: [empty]");
    }
    
    // Save new credentials
    ssid = newSsid;
    password = newPass;
    saveWifiCredentials();
    
    // Send success response
    String html = "<!DOCTYPE html><html><head><title>WiFi Saved</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>body{font-family:Arial,sans-serif;margin:20px;text-align:center;background:#f0f0f0}";
    html += ".container{max-width:400px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}";
    html += "h1{color:#28a745}h2{color:#333}</style></head><body>";
    html += "<div class='container'>";
    html += "<h1>✅ WiFi Settings Saved!</h1>";
    html += "<h2>Connecting to: " + newSsid + "</h2>";
    html += "<p>The device will restart and attempt to connect to your WiFi network.</p>";
    html += "<p>If successful, you can access the seismometer dashboard at its new IP address.</p>";
    html += "<p>If connection fails, the device will return to Access Point mode.</p>";
    html += "</div>";
    html += "<script>setTimeout(function(){window.close();},5000);</script>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
    
    delay(2000);
    Serial.println("Rebooting to apply new WiFi settings...");
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Missing SSID parameter");
  }
}

void saveWifiCredentials() {
  Serial.println(F("Saving WiFi credentials to EEPROM..."));
  
  // Clear the EEPROM region for credentials before writing
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }

  // Write SSID to EEPROM
  Serial.print(F("Writing SSID: "));
  Serial.println(ssid);
  for (int i = 0; i < ssid.length(); ++i) {
    EEPROM.write(SSID_ADDR + i, ssid[i]);
  }
  
  // Write Password to EEPROM
  Serial.println(F("Writing Password."));
  for (int i = 0; i < password.length(); ++i) {
    EEPROM.write(PASS_ADDR + i, password[i]);
  }
  
  if (EEPROM.commit()) {
    Serial.println(F("Credentials saved successfully."));
  } else {
    Serial.println(F("ERROR: Failed to commit credentials to EEPROM."));
  }
}

void loadWifiCredentials() {
  Serial.println(F("Loading WiFi credentials from EEPROM..."));
  
  // Read SSID
  char ssid_buf[SSID_LEN] = {0};
  for (int i = 0; i < SSID_LEN; ++i) {
    ssid_buf[i] = EEPROM.read(SSID_ADDR + i);
  }
  
  // Read Password
  char pass_buf[PASS_LEN] = {0};
  for (int i = 0; i < PASS_LEN; ++i) {
    pass_buf[i] = EEPROM.read(PASS_ADDR + i);
  }

  String loadedSsid = String(ssid_buf);
  String loadedPass = String(pass_buf);

  if (loadedSsid.length() > 0) {
    ssid = loadedSsid;
    Serial.print(F("Loaded SSID: "));
    Serial.println(ssid);
  } else {
    Serial.println(F("No SSID found in EEPROM, using default."));
  }

  if (loadedPass.length() > 0) {
    password = loadedPass;
    Serial.println(F("Loaded password from EEPROM."));
  } else {
    Serial.println(F("No password found in EEPROM, using default."));
  }
}

String getSensorDataJson() {
  // Calculate current deviation magnitude for JSON
  float x_dev = abs(x_accel - x_baseline);
  float y_dev = abs(y_accel - y_baseline);
  float z_dev = abs(z_accel - z_baseline);
  if (x_dev < noise_threshold) x_dev = 0;
  if (y_dev < noise_threshold) y_dev = 0;
  if (z_dev < noise_threshold) z_dev = 0;
  float dev_mag = sqrt(x_dev*x_dev + y_dev*y_dev + z_dev*z_dev);
  int current_mercalli = calculateMercalli(dev_mag);

  // Create JSON response
  String json = "{";
  json += "\"mercalli_peak\":" + String(mercalli_peak) + ",";
  json += "\"mercalli_now\":" + String(current_mercalli) + ",";
  json += "\"x_peak\":" + String(x_peak) + ",";
  json += "\"y_peak\":" + String(y_peak) + ",";
  json += "\"z_peak\":" + String(z_peak) + ",";
  json += "\"dev_mag_peak\":" + String(deviation_magnitude_peak) + ",";
  json += "\"x_now\":" + String(x_dev) + ",";
  json += "\"y_now\":" + String(y_dev) + ",";
  json += "\"z_now\":" + String(z_dev) + ",";
  json += "\"dev_mag_now\":" + String(dev_mag) + "}";
  
  return json;
}

void handleReset() {
  resetPeakValues();
  server.send(204, "text/plain", ""); // 204 No Content is a good response for a successful action with no reply body
}