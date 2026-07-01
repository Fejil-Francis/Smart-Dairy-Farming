# 🐄 Smart Dairy Farming

An IoT-based cattle health monitoring system that enables real-time monitoring of cattle health and activity using ESP32 and multiple sensors. The system collects sensor data, transmits it over Wi-Fi, and stores it in Google Sheets through Google Apps Script for easy access and analysis.

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

* ESP32
* MAX30102 (Heart Rate & SpO₂ Sensor)
* MPU6050 (Accelerometer & Gyroscope)
* Temperature Sensor
* Breadboard and Jumper Wires
* Rechargeable Battery

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

1. Connect all sensors to the ESP32.
2. Upload the firmware using the Arduino IDE.
3. Configure the Wi-Fi SSID and password.
4. Deploy the Google Apps Script as a Web App.
5. Update the Web App URL in the ESP32 code.
6. Power on the device.
7. Sensor data will be transmitted to Google Sheets in real time.

---

## 📊 Data Flow

```text
Sensors
   ↓
ESP32
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
* ✅ Sensors integrated with ESP32
* ✅ Wi-Fi communication implemented
* ✅ Google Apps Script configured
* ⏳ Ready for real-time data collection and analysis

---

