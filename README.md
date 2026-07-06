# 🐄 Smart Dairy Farming

An IoT-based cattle health monitoring system that enables real-time monitoring of cattle health and activity using ESP8266 and multiple sensors. The system collects sensor data, transmits it over Wi-Fi, and stores it in Google Sheets through Google Apps Script for easy access and analysis.

---

## ✨ Features

* Real-time cattle health monitoring
* Heart rate and SpO₂ monitoring using MAX30102
* Motion and activity detection using MPU6050
* Wi-Fi-based data transmission
* Automatic data storage in Google Sheets
* Remote monitoring through a web browser
* Low-cost and user-friendly solution

---

## 🛠️ Hardware Used

* ESP8266
* MAX30102 (Heart Rate & SpO₂ Sensor)
* MPU6050 (Accelerometer & Gyroscope)
* Temperature Sensor
* Breadboard and Jumper Wires
* Rechargeable Battery
* HW-668 booster module

---

## 💻 Software & Technologies

* Arduino IDE
* Embedded C/C++
* Wi-Fi
* HTTP
* Google Apps Script
* Google Sheets

---

## 📂 Project Structure

```text
Smart-Dairy-Farming/
├── src/
├── docs/
├── README.md
```

---

## 🚀 Usage

1. Connect the MAX30102, MPU6050, buzzer, and all other required sensors to the ESP8266 NodeMCU according to the circuit diagram.
2. Connect the battery to the input terminals (VIN+/VIN−) of the DC-DC booster (boost converter).
3. Adjust the booster output to the required voltage (typically 5 V for the ESP8266 VIN pin or 3.3 V if powering the regulated 3.3 V rail directly).
4. Connect the booster output to the ESP8266 power input (VIN and GND, or 3.3 V and GND as appropriate).
5. Install the Arduino IDE on your computer.
6. Install the ESP8266 board package and all the required libraries.
7. Open the project in the Arduino IDE.
8. Configure the Wi-Fi SSID and password in the source code.
9. Deploy the Google Apps Script as a Web App.
10. Update the Google Apps Script Web App URL (or Script ID) in the ESP8266 source code.
11. Connect the ESP8266 to the computer using a USB cable and upload the firmware through the Arduino IDE.
12. Disconnect the USB cable (if required) and power the system using the battery through the booster module.
13. Wait for the ESP8266 to connect to the configured Wi-Fi network.
14. The ESP8266 will acquire data from the MAX30102 and MPU6050 sensors and transmit the readings to Google Sheets in real time.
15. Open the Google Sheet or the ESP8266 web dashboard to monitor the live sensor data.


---

## 📊 Data Flow

```text
Sensors
   ↓
ESP8266
   ↓ (Wi-Fi + HTTP)
Google Apps Script
   ↓
Google Sheets
   ↓
Monitoring Dashboard
```

---

## 📌 Current Status

* ✅ Hardware setup completed
* ✅ Sensors integrated with ESP8266
* ✅ Wi-Fi communication implemented
* ✅ Google Apps Script configured
* ⏳ Ready for real-time data collection and analysis

---

