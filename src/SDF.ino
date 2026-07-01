#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

// --- Configuration ---
const char* ssid = "vivo";
const char* password = "fejileee";
const String scriptID = "AKfycbz71D1hCc6DUin-FpbYAlMChVa-GqL16mMAljaexcDV_KKBq_BlNuk9Dasa_zGE6AL6iA";

ESP8266WebServer server(80);
Adafruit_MPU6050 mpu;
MAX30105 particleSensor;

uint32_t irBuffer[100], redBuffer[100];
int32_t spo2, heartRate;
int8_t validSPO2, validHeartRate;
unsigned long lastCloudUpload = 0;

// --- Dashboard HTML ---
const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html><html><head><meta charset="UTF-8"><title>Cattle Monitor</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<style>
  body { font-family: sans-serif; background: #0d1117; color: #c9d1d9; text-align: center; margin:0; }
  .header { background: #161b22; padding: 20px; border-bottom: 2px solid #58a6ff; }
  .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; padding: 20px; }
  .card { background: #161b22; padding: 25px; border-radius: 12px; border: 1px solid #30363d; box-shadow: 0 4px 8px rgba(0,0,0,0.3); }
  .label { color: #8b949e; font-size: 0.8rem; margin-bottom: 10px; }
  .val { font-size: 2.2rem; color: #58a6ff; font-weight: bold; }
</style></head><body>
  <div class="header"><h1>CATTLE TELEMETRY LIVE</h1></div>
  <div class="grid">
    <div class="card"><div class="label">HEART RATE (BPM)</div><div class="val" id="bpm">--</div></div>
    <div class="card"><div class="label">OXYGEN (SpO2 %)</div><div class="val" id="spo2">--</div></div>
    <div class="card"><div class="label">TEMP (°C)</div><div class="val" id="temp">--</div></div>
    <div class="card"><div class="label">MOTION (G-FORCE)</div><div class="val" id="mag">--</div></div>
  </div>
  <script>
    setInterval(() => {
      fetch('/data').then(r => r.json()).then(d => {
        document.getElementById('bpm').innerText = d.bpm > 0 ? d.bpm : "Scanning...";
        document.getElementById('spo2').innerText = d.spo2 > 0 ? d.spo2 : "--";
        document.getElementById('temp').innerText = d.temp;
        let m = Math.sqrt(d.ax**2 + d.ay**2 + d.az**2).toFixed(2);
        document.getElementById('mag').innerText = m;
      }).catch(e => console.log("Waiting for data..."));
    }, 1000);
  </script>
</body></html>
)=====";

// --- Functions must be defined BEFORE setup() ---

void handleRoot() { 
  server.send(200, "text/html", INDEX_HTML); 
}

void handleData() {
  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);
  float bodyTemp = particleSensor.readTemperature();
  
  String json = "{";
  json += "\"bpm\":" + String(validHeartRate ? heartRate : 0) + ",";
  json += "\"spo2\":" + String(validSPO2 ? spo2 : 0) + ",";
  json += "\"temp\":" + String(bodyTemp, 1) + ",";
  json += "\"ax\":" + String(a.acceleration.x) + ",";
  json += "\"ay\":" + String(a.acceleration.y) + ",";
  json += "\"az\":" + String(a.acceleration.z);
  json += "}";
  server.send(200, "application/json", json);
}

void sendToGoogle(int hr, int ox, float temp, float ax, float ay, float az, float gx, float gy, float gz) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  
  String url = "https://script.google.com/macros/s/" + scriptID + "/exec?";
  url += "bpm=" + String(hr) + "&spo2=" + String(ox) + "&temp=" + String(temp, 1);
  url += "&ax=" + String(ax) + "&ay=" + String(ay) + "&az=" + String(az);
  url += "&gx=" + String(gx) + "&gy=" + String(gy) + "&gz=" + String(gz);

  if (http.begin(client, url)) {
    int code = http.GET();
    Serial.println("Google Sheets Sync: " + String(code));
    http.end();
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(4, 5);
  
  if(!mpu.begin()) { Serial.println("MPU6050 Missing"); while(1) yield(); }
  if(!particleSensor.begin(Wire, I2C_SPEED_FAST)) { Serial.println("MAX30102 Missing"); while(1) yield(); }
  
  particleSensor.setup(60, 4, 2, 100, 411, 4096);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nIP: " + WiFi.localIP().toString());
  
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  
  // Fill initial buffer
  for (byte i = 0; i < 100; i++) {
    while (!particleSensor.available()) particleSensor.check();
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample();
  }
}

void loop() {
  server.handleClient();
  yield();

  // Sliding window update
  for (byte i = 25; i < 100; i++) {
    redBuffer[i-25] = redBuffer[i];
    irBuffer[i-25] = irBuffer[i];
  }
  for (byte i = 75; i < 100; i++) {
    while (!particleSensor.available()) { particleSensor.check(); yield(); }
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample();
  }

  maxim_heart_rate_and_oxygen_saturation(irBuffer, 100, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  if (millis() - lastCloudUpload > 15000) {
    sensors_event_t a, g, t;
    mpu.getEvent(&a, &g, &t);
    sendToGoogle(validHeartRate ? heartRate : 0, validSPO2 ? spo2 : 0, 
                 particleSensor.readTemperature(), a.acceleration.x, a.acceleration.y, a.acceleration.z, 
                 g.gyro.x, g.gyro.y, g.gyro.z);
    lastCloudUpload = millis();
  }
}