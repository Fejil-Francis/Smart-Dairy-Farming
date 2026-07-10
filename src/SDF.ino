#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include "MAX30105.h"
#include "spo2_algorithm.h"

//=========================
// WiFi Configuration
//=========================
const char* ssid = "";
const char* password = "";

// Google Apps Script ID
String scriptID = "AKfycbz71D1hCc6DUin-FpbYAlMChVa-GqL16mMAljaexcDV_KKBq_BlNuk9Dasa_zGE6AL6iA";

//=========================
// Objects
//=========================
ESP8266WebServer server(80);

Adafruit_MPU6050 mpu;
MAX30105 particleSensor;

//=========================
// Variables
//=========================
uint32_t irBuffer[100];
uint32_t redBuffer[100];

int32_t spo2 = 0;
int32_t heartRate = 0;

int8_t validSPO2 = 0;
int8_t validHeartRate = 0;

unsigned long lastUpload = 0;

//=========================
// Dashboard HTML
//=========================
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Cattle Monitor</title>
<style>
body{ background:#111827; color:white; font-family:Arial; text-align:center; }
.card{ display:inline-block; width:220px; padding:20px; margin:15px; background:#1f2937; border-radius:10px; font-size:24px; }
.value{ font-size:36px; color:#00d4ff; }
</style>
</head>
<body>
<h1>Cattle Health Monitoring</h1>
<div class="card">Heart Rate<div class="value" id="hr">--</div></div>
<div class="card">SpO₂<div class="value" id="spo2">--</div></div>
<div class="card">Temperature<div class="value" id="temp">--</div></div>
<div class="card">Acceleration<div class="value" id="acc">--</div></div>
<script>
setInterval(function(){
fetch("/data")
.then(r=>r.json())
.then(data=>{
document.getElementById("hr").innerHTML = data.hr > 0 ? data.hr + " BPM" : "--";
document.getElementById("spo2").innerHTML = data.spo2 > 0 ? data.spo2 + " %" : "--";
document.getElementById("temp").innerHTML = data.temp + " °C";
let mag = Math.sqrt(data.ax*data.ax + data.ay*data.ay + data.az*data.az);
document.getElementById("acc").innerHTML = mag.toFixed(2);
});
}, 1000);
</script>
</body>
</html>
)rawliteral";

//=========================
// Function Prototypes
//=========================
void handleRoot();
void handleData();
void sendToGoogle(float,float,float,float,float,float,float,float,float);
void fillSensorBuffer();

//=========================
// Setup
//=========================
void setup()
{
    Serial.begin(115200);

    // SDA = D2, SCL = D1
    Wire.begin(D2, D1);
    Wire.setClock(400000); 

    Serial.println("\nInitializing MPU6050...");
    if(!mpu.begin()) {
        Serial.println("MPU6050 NOT FOUND");
    }

    Serial.println("Initializing MAX30102...");
    if(!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println("MAX30102 NOT FOUND");
    }

    // Settings matched precisely for the Maxim algorithm sample expectations
    particleSensor.setup(60, 4, 2, 100, 411, 4096);

    Serial.println("Connecting WiFi...");
    WiFi.begin(ssid, password);
    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected!");

    server.on("/", handleRoot);
    server.on("/data", handleData);
    server.begin();

    fillSensorBuffer();
}

//====================================================
// Web Server Handlers
//====================================================
void handleRoot() {
    server.send_P(200, "text/html", webpage);
}

void handleData() {
    sensors_event_t accel, gyro, tempEvent;
    mpu.getEvent(&accel, &gyro, &tempEvent);
    float sensorTemp = particleSensor.readTemperature();

    String json = "{";
    json += "\"hr\":" + String(validHeartRate ? heartRate : 0) + ",";
    json += "\"spo2\":" + String(validSPO2 ? spo2 : 0) + ",";
    json += "\"temp\":" + String(sensorTemp, 1) + ",";
    json += "\"ax\":" + String(accel.acceleration.x, 2) + ",";
    json += "\"ay\":" + String(accel.acceleration.y, 2) + ",";
    json += "\"az\":" + String(accel.acceleration.z, 2);
    json += "}";

    server.send(200, "application/json", json);
}

//====================================================
// Google Sheets Upload (With Built-in Redirection Follow)
//====================================================
//====================================================
// Google Sheets Upload (Fixed Redirect Method)
//====================================================
void sendToGoogle(float hr, float spo, float temp, float ax, float ay, float az, float gx, float gy, float gz) {
    if (WiFi.status() != WL_CONNECTED) return;

    std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure);
    client->setInsecure();

    HTTPClient http;
    http.setTimeout(4000); 

    // Explicitly tell the client to collect the "Location" header for redirects
    const char* headerKeys[] = {"Location"};
    http.collectHeaders(headerKeys, 1);

    String url = "https://script.google.com/macros/s/" + scriptID + "/exec?";
    url += "bpm=" + String(hr);
    url += "&spo2=" + String(spo);
    url += "&temp=" + String(temp, 1);
    url += "&ax=" + String(ax, 2);
    url += "&ay=" + String(ay, 2);
    url += "&az=" + String(az, 2);
    url += "&gx=" + String(gx, 2);
    url += "&gy=" + String(gy, 2);
    url += "&gz=" + String(gz, 2);

    Serial.println("Contacting Google Web App...");
    if (http.begin(*client, url)) {
        int code = http.GET();
        Serial.print("Initial Code: "); Serial.println(code);

        // Handle Google Apps Script 302 Redirect via getLocation()
        if (code == 302 || code == 301) {
            String redirectUrl = http.getLocation();
            if (redirectUrl.length() > 0) {
                http.end(); // Close connection before following
                Serial.println("Following Redirect...");
                if (http.begin(*client, redirectUrl)) {
                    code = http.GET();
                    Serial.print("Final Sheet Code: "); Serial.println(code);
                }
            }
        }
        http.end();
    }
}

// Anti-Freeze Buffered Reader
void fillSensorBuffer() {
    for(int i = 0; i < 100; i++) {
        unsigned long waitStart = millis();
        while(!particleSensor.available()) {
            particleSensor.check();
            server.handleClient(); 
            yield();
            if (millis() - waitStart > 100) break; // Latch Break if sensor misses a beat
        }
        redBuffer[i] = particleSensor.getRed();
        irBuffer[i] = particleSensor.getIR();
        particleSensor.nextSample();
    }
}

//====================================================
// Main Loop
//====================================================
void loop()
{
    server.handleClient();
    yield();

    // Shift data window
    for (byte i = 25; i < 100; i++) {
        redBuffer[i - 25] = redBuffer[i];
        irBuffer[i - 25] = irBuffer[i];
    }

    // Read 25 fresh samples safely
    for (byte i = 75; i < 100; i++) {
        unsigned long waitStart = millis();
        while (!particleSensor.available()) {
            particleSensor.check();
            server.handleClient(); 
            yield();
            if (millis() - waitStart > 100) break; 
        }
        redBuffer[i] = particleSensor.getRed();
        irBuffer[i] = particleSensor.getIR();
        particleSensor.nextSample();
    }

    // Run processing calculations
    maxim_heart_rate_and_oxygen_saturation(
        irBuffer, 100, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate
    );

    // Collect MPU6050 Metrics
    sensors_event_t accel, gyro, tempEvent;
    mpu.getEvent(&accel, &gyro, &tempEvent);

    // Serial Terminal Output
    Serial.println("===============================");
    Serial.print("HR: "); Serial.println(validHeartRate ? String(heartRate) : "Calculating...");
    Serial.print("SpO2: "); Serial.println(validSPO2 ? String(spo2) : "Calculating...");

    // Upload precisely every 3000ms
    if (millis() - lastUpload >= 3000) {
        sendToGoogle(
            validHeartRate ? heartRate : 0,
            validSPO2 ? spo2 : 0,
            particleSensor.readTemperature(),
            accel.acceleration.x, accel.acceleration.y, accel.acceleration.z,
            gyro.gyro.x, gyro.gyro.y, gyro.gyro.z
        );
        lastUpload = millis();
        fillSensorBuffer(); // Clear lag anomalies
    }

    delay(15);
}