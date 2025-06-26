# ESP32 Mercalli Seismometer

A highly sensitive, WiFi and BLE-enabled seismometer based on the ESP32 microcontroller and an ADXL345 accelerometer. It measures vibrations, tracks peak values, calculates the Mercalli intensity scale, and provides real-time data via an OLED display, a web interface, and a BLE service.

![OLED Display](https://i.imgur.com/your-image-link.jpg) <!-- Placeholder: You can add a screenshot of your project here -->

## Features

- **Real-time Vibration Sensing**: Uses an ADXL345 accelerometer to detect vibrations on three axes.
- **OLED Display**: A 128x64 SSD1306 OLED screen shows real-time peak deviations and Mercalli intensity.
- **Peak & Mercalli Tracking**: Intelligently tracks and displays the peak deviation from baseline and the corresponding Mercalli intensity value.
- **Automatic Calibration**: Performs a software-based calibration on startup to establish a zero-gravity baseline and determine the ambient noise threshold.
- **Web Interface**: Hosts a web server that provides a live dashboard to monitor seismic data from any device on the same network.
- **Bluetooth Low Energy (BLE)**: Broadcasts sensor data via a BLE service, allowing for connection from mobile apps or other BLE-enabled devices. Includes a simple HTML viewer for BLE data.
- **Persistent WiFi Configuration**: WiFi credentials (SSID and password) can be set via the serial monitor and are stored persistently in EEPROM. The device can also run in an offline mode if WiFi is not available.
- **Hardware Reset**: A physical button allows for the manual reset of peak values and the baseline.
- **Robust Serial Interface**: Provides commands for status checks, manual resets, recalibration, and WiFi configuration.

## Hardware Requirements

- **ESP32 Development Board**: Any standard ESP32 board.
- **ADXL345 Accelerometer**: A 3-axis digital accelerometer module.
- **SSD1306 OLED Display**: A 128x64 I2C OLED display.
- **Push Button**: A standard momentary push button for resetting peak values.
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
- `espressif/arduino-esp32` (for WiFi, WebServer)
- `nkolban/ESP32 BLE Arduino`

### Installation

1.  **Clone the Repository**:
    ```bash
    git clone https://github.com/your-username/your-repo-name.git
    ```
2.  **Open in PlatformIO**: Open the cloned folder in Visual Studio Code with the PlatformIO IDE extension installed.
3.  **Build & Upload**: Connect your ESP32 board, and use the PlatformIO controls to build and upload the firmware.

## Usage

### First Boot: Calibration

On the first boot, the device will automatically start a calibration sequence.
- **Keep the device perfectly still** on a level surface.
- The OLED display and Serial Monitor will show the progress.
- This process measures the sensor's baseline at rest and calculates the ambient noise level to prevent false readings.

### OLED Display

The display shows:
- **Peak Deviations**: The maximum detected acceleration deviation (in m/s²) for the X, Y, and Z axes since the last reset.
- **Mercalli Intensity**: The peak intensity (large number) and the current real-time intensity (small number).
- **Status**: During startup and calibration, it shows status messages like "CALIBRATING..." or the device's IP address.

### Web Interface

1.  After connecting to your WiFi network, the ESP32's IP address will be printed to the Serial Monitor and shown briefly on the OLED.
2.  Open a web browser on a device connected to the same network and navigate to `http://<ESP32_IP_ADDRESS>`.
3.  The dashboard will display live data from the seismometer.

### BLE Interface

1.  Use a BLE scanner app (like nRF Connect or LightBlue) on your phone or computer.
2.  Scan for devices and connect to the one named **"Seismometer"**.
3.  Subscribe to the characteristic with UUID `beb5483e-36e1-4688-b7f5-ea07361b26a8` to receive live JSON data.
4.  You can also access a simple BLE data viewer by navigating to `http://<ESP32_IP_ADDRESS>/ble`.

### Serial Commands

Connect to the ESP32 using a serial monitor at **115200 baud**. The following commands are available:

- `STATUS`: Prints the current WiFi, BLE, and calibration status.
- `RESET`: Resets the peak values and re-establishes the baseline. (Same as pressing the hardware button).
- `CALIBRATE`: Starts a new manual calibration sequence.
- `SSID <your_ssid>`: Sets a new WiFi SSID and saves it. The device will reboot.
- `PASS <your_password>`: Sets a new WiFi password and saves it. The device will reboot.
- `BOOT`: Restarts the ESP32.

## Contributing

Contributions, issues, and feature requests are welcome! Feel free to check the [issues page](https://github.com/your-username/your-repo-name/issues).

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.
