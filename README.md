# ESP32 Mercalli Seismometer

A highly sensitive, WiFi and BLE-enabled seismometer based on the ESP32 microcontroller and an ADXL345 accelerometer. It measures vibrations, tracks peak values, calculates the Mercalli intensity scale, and provides real-time data via an OLED display, a web interface, and a BLE service. Features advanced event logging with NTP time synchronization and intelligent WiFi provisioning.

![OLED Display](https://i.imgur.com/your-image-link.jpg) <!-- Placeholder: You can add a screenshot of your project here -->

## Features

### Core Functionality
- **Real-time Vibration Sensing**: Uses an ADXL345 accelerometer to detect vibrations on three axes with noise filtering
- **OLED Display**: A 128x64 SSD1306 OLED screen shows real-time peak deviations and Mercalli intensity
- **Peak & Mercalli Tracking**: Intelligently tracks and displays the peak deviation from baseline and the corresponding Mercalli intensity value
- **Automatic Calibration**: Performs a software-based calibration on startup to establish a zero-gravity baseline and determine the ambient noise threshold
- **Moving Baseline**: Adaptive baseline that slowly adjusts to environmental changes while detecting genuine seismic activity

### Connectivity & Interfaces
- **Robust WiFi Management**: Automatic connection with fallback to Access Point mode for easy configuration
- **WiFi Provisioning Portal**: User-friendly captive portal with network scanning and clickable SSID selection
- **Web Interface**: Comprehensive dashboard with real-time data visualization and mobile-responsive design
- **Bluetooth Low Energy (BLE)**: Broadcasts sensor data via a BLE service with dedicated web viewer
- **Persistent WiFi Configuration**: WiFi credentials stored in EEPROM with serial and web configuration options

### Advanced Event Logging
- **NTP Time Synchronization**: Automatic time sync from multiple NTP servers for accurate timestamping
- **Intelligent Event Detection**: Logs significant seismic events (Mercalli III+) with smart filtering to avoid spam
- **Event Log Storage**: Circular buffer storing up to 50 events with timestamps, magnitudes, and peak deviations
- **Event Log Web Interface**: Dedicated page for viewing and managing logged seismic events
- **Event Log Management**: Clear event log functionality via web interface and serial commands

### User Interface & Control
- **Hardware Reset**: Physical button for manual reset of peak values and baseline
- **Comprehensive Serial Interface**: Rich command set for status, configuration, and control
- **Status Monitoring**: Detailed system status reporting including WiFi, BLE, calibration, and event logging status
- **Light Theme UI**: Clean, modern web interface optimized for both desktop and mobile devices

## Hardware Requirements

- **ESP32 Development Board**: Any standard ESP32 board
- **ADXL345 Accelerometer**: A 3-axis digital accelerometer module
- **SSD1306 OLED Display**: A 128x64 I2C OLED display
- **Push Button**: A standard momentary push button for resetting peak values
- **Breadboard and Jumper Wires**

### Wiring

| ESP32 Pin | Component Pin      |
|-----------|--------------------|
| `GND`     | ADXL345 `GND`      |
| `3V3`     | ADXL345 `VCC`      |
| `GPIO 21` | ADXL345 `SDA`      |
| `GPIO 22` | ADXL345 `SCL`      |
| `GND`     | SSD1306 `GND`      |
| `3V3`     | SSD1306 `VCC`      |
| `GPIO 21` | SSD1306 `SDA`      |
| `GPIO 22` | SSD1306 `SCL`      |
| `GPIO 4`  | Push Button (one leg) |
| `GND`     | Push Button (other leg) |

*Note: The internal pull-up resistor is used for the button on `GPIO 4`.*

## Software & Setup

This project is built using the **PlatformIO IDE** with the Arduino framework.

### Libraries

The following libraries are required and are automatically managed by PlatformIO via the `platformio.ini` file:
- `adafruit/Adafruit GFX Library`
- `adafruit/Adafruit SSD1306`
- `adafruit/Adafruit ADXL345`
- `adafruit/Adafruit Unified Sensor`
- `adafruit/Adafruit BusIO`
- `espressif/arduino-esp32` (for WiFi, WebServer)
- `nkolban/ESP32 BLE Arduino`
- `EEPROM` (for persistent WiFi storage)

### Installation

1.  **Clone the Repository**:
    ```bash
    git clone https://github.com/your-username/your-repo-name.git
    ```
2.  **Open in PlatformIO**: Open the cloned folder in Visual Studio Code with the PlatformIO IDE extension installed.
3.  **Build & Upload**: Connect your ESP32 board, and use the PlatformIO controls to build and upload the firmware.

## Usage

### First Boot: WiFi Setup

On first boot, if no WiFi credentials are configured, the device will start in **Access Point mode**:
1. **Connect to the AP**: Look for a WiFi network named "Seismometer-XXXXXX" (where XXXXXX is part of the device's MAC address)
2. **Open Configuration Portal**: Navigate to `http://192.168.4.1` in your browser
3. **Select WiFi Network**: Choose your WiFi network from the scanned list or enter manually
4. **Enter Password**: Provide the WiFi password
5. **Save & Connect**: The device will restart and connect to your WiFi network

### Calibration

After WiFi setup, the device automatically starts calibration:
- **Keep the device perfectly still** on a level surface during calibration
- The OLED display and Serial Monitor show the progress
- This process measures the sensor's baseline and calculates ambient noise levels
- The IP address is displayed during calibration for immediate web access

### OLED Display

The display shows:
- **Peak Deviations**: Maximum detected acceleration deviation (in m/s²) for X, Y, and Z axes since last reset
- **Mercalli Intensity**: Peak intensity (large number) and current real-time intensity (small number)
- **Baseline Status**: During startup, shows baseline establishment progress
- **WiFi Information**: IP address during calibration and setup

### Web Interface

#### Main Dashboard (`http://<ESP32_IP>/`)
- **Real-time Data**: Live sensor readings and Mercalli intensity
- **Peak Values**: Current peak deviations and maximum Mercalli reading
- **Event Information**: Recent event count and last logged event
- **Auto-refresh**: Page updates automatically every 5 seconds
- **Mobile Responsive**: Optimized for both desktop and mobile devices

#### Event Log (`http://<ESP32_IP>/events`)
- **Event History**: Chronological list of seismic events (Mercalli III+)
- **Detailed Information**: Timestamp, Mercalli intensity, peak deviations, and magnitude
- **Color Coding**: Visual indicators for event severity levels
- **Event Management**: Clear all events with confirmation dialog
- **JSON API**: Access raw event data at `/events?format=json`

#### BLE Viewer (`http://<ESP32_IP>/ble`)
- **Alternative Interface**: Web-based BLE data viewer
- **Real-time Updates**: Live data display without BLE connection required

#### WiFi Configuration (`http://<ESP32_IP>/config`)
- **Network Scanning**: Automatic WiFi network detection
- **Signal Strength**: Visual indicators for network quality
- **Manual Entry**: Fallback for hidden networks
- **Live Data**: Continue monitoring while configuring WiFi

### BLE Interface

1.  **Scan for Device**: Use a BLE scanner app (nRF Connect, LightBlue, etc.)
2.  **Connect to "Seismometer"**: Look for the device name in scan results
3.  **Subscribe to Data**: UUID `beb5483e-36e1-4688-b7f5-ea07361b26a8` for live JSON data
4.  **Send Reset Commands**: UUID `ec0e0001-36e1-4688-b7f5-ea07361b26a8` to reset peak values

### Event Logging System

#### Automatic Event Detection
- **Time Synchronization**: Automatic NTP sync when connected to internet
- **Intelligent Filtering**: Smart algorithm prevents spam while capturing genuine events
- **Event Criteria**:
  - Mercalli V+ events: Always logged if higher than last OR 10+ seconds elapsed
  - Mercalli III-IV events: Logged if higher than last AND 10+ seconds elapsed
  - Significant jumps: Always logged if 2+ Mercalli level increase regardless of time

#### Event Storage
- **Circular Buffer**: Stores up to 50 events in memory
- **Persistent During Runtime**: Events maintained until device restart or manual clear
- **Detailed Logging**: Each event includes timestamp, Mercalli level, individual axis peaks, and magnitude

### Serial Commands

Connect via serial monitor at **115200 baud**. Available commands:

- `STATUS`: Comprehensive system status (WiFi, BLE, calibration, event logging)
- `RESET`: Reset peak values and re-establish baseline
- `CLEAREVENTS`: Clear all logged seismic events
- `CALIBRATE`: Start manual calibration sequence
- `SSID <your_ssid>`: Set WiFi SSID and save to EEPROM (triggers reboot)
- `PASS <your_password>`: Set WiFi password and save to EEPROM (triggers reboot)
- `BOOT`: Restart the ESP32

### Physical Controls

- **Reset Button (GPIO 4)**: Press to reset peak values and baseline (same as `RESET` command)
- **Debounced Input**: Button presses are properly debounced to prevent false triggers

## Configuration & Customization

### Mercalli Intensity Thresholds

The Mercalli scale thresholds can be adjusted in `main.cpp`:
```cpp
const float MERCALLI_3_THRESHOLD = 0.4;   // III - Weak
const float MERCALLI_4_THRESHOLD = 0.7;   // IV - Light
const float MERCALLI_5_THRESHOLD = 1.2;   // V - Moderate
// ... etc
```

### Event Logging Parameters

Event logging behavior can be customized:
```cpp
const unsigned long MIN_EVENT_INTERVAL = 10000;  // Minimum 10 seconds between events
#define MAX_EVENTS 50  // Maximum number of stored events
```

### Network Settings

NTP servers and timing can be configured:
```cpp
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const char* ntpServer3 = "time.google.com";
```

## API Endpoints

### REST API
- `GET /` - Main dashboard (HTML)
- `GET /data` - Current sensor data (JSON)
- `POST /reset` - Reset peak values
- `GET /events` - Event log page (HTML)
- `GET /events?format=json` - Event log data (JSON)
- `POST /clearevents` - Clear event log
- `GET /ble` - BLE viewer page (HTML)
- `GET /config` - WiFi configuration page (HTML)
- `POST /save` - Save WiFi credentials

### JSON Data Format

Sensor data API response:
```json
{
  "mercalli_peak": 3,
  "mercalli_now": 1,
  "x_peak": 0.245,
  "y_peak": 0.189,
  "z_peak": 0.312,
  "dev_mag_peak": 0.445,
  "x_now": 0.012,
  "y_now": 0.008,
  "z_now": 0.015,
  "dev_mag_now": 0.021,
  "eventCount": 5,
  "timeSync": true,
  "lastEvent": {
    "timestamp": "2025-07-01 14:30:25 UTC",
    "mercalli": 4
  }
}
```

## Troubleshooting

### WiFi Connection Issues
- **Check Credentials**: Use `STATUS` command to verify stored SSID/password
- **Signal Strength**: Ensure ESP32 is within WiFi range
- **Fallback Mode**: Device automatically starts AP mode if WiFi fails
- **Manual Reset**: Use `SSID` and `PASS` commands to reconfigure

### Calibration Problems
- **Keep Still**: Ensure device is motionless during calibration
- **Level Surface**: Place on stable, level surface
- **Recalibrate**: Use `CALIBRATE` command to repeat process
- **Check Wiring**: Verify ADXL345 connections

### Event Logging Issues
- **Time Sync**: Requires internet connection for NTP synchronization
- **Check Status**: Use `STATUS` command to verify time sync status
- **Manual Clear**: Use `CLEAREVENTS` if log appears corrupted

### Performance Optimization
- **Upload Speed**: Configured for 921600 baud for faster uploads
- **Build Optimization**: Compiler optimizations enabled for better performance
- **Memory Usage**: Monitor RAM usage if extending functionality

## Contributing

Contributions, issues, and feature requests are welcome! Feel free to check the [issues page](https://github.com/your-username/your-repo-name/issues).

### Development Notes
- Built with PlatformIO and Arduino framework
- Uses ESP32's dual-core architecture efficiently
- Web interface built with vanilla HTML/CSS/JavaScript for minimal overhead
- Event logging designed for 24/7 operation
- Comprehensive error handling and recovery mechanisms

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.

## Acknowledgments

- Adafruit for excellent sensor libraries
- ESP32 community for WiFi and BLE examples
- PlatformIO team for the excellent development environment
