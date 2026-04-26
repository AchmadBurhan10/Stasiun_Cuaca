#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DFRobot_EnvironmentalSensor.h>
#include <Wire.h>

// --- 1. KONFIGURASI WIFI & SERVER (Sesuaikan IP Laptop E2 lo) ---
const char* ssid = "NAMA_WIFI";
const char* password = "PASSWORD_WIFI";
const char* mqtt_server = "IP_SERVER"; 
const int   mqtt_port = 1883;

// --- 2. KONFIGURASI PIN & SENSOR ---
constexpr byte I2C_SDA_PIN = 8;
constexpr byte I2C_SCL_PIN = 9;
DFRobot_EnvironmentalSensor Sensor_Lingkungan(SEN050X_DEFAULT_DEVICE_ADDRESS, &Wire);

// --- 3. TIMING CONTROL (Agar laptop E2 tidak overload) ---
unsigned long Waktu_Terakhir_Baca = 0;
unsigned long Waktu_Terakhir_Kirim = 0;
const long Interval_Baca = 1500;  // Baca & Filter tiap 2 detik
const long Interval_Kirim = 15000; // Kirim ke IoT tiap 30 detik

// --- 4. LOGIKA FILTER (Sesuai Class Mas Burhan) ---
constexpr int WINDOW_SIZE = 5;
constexpr float EMA_ALPHA = 0.2f;

class SensorFilter {
private:
  float buffer[WINDOW_SIZE];
  int bufferIndex = 0;
  bool bufferFilled = false;
  float emaValue = 0.0f;
  bool isFirstReading = true;

public:
  SensorFilter() {
    for (int i = 0; i < WINDOW_SIZE; i++) buffer[i] = 0.0f;
  }

  float process(float rawValue) {
    buffer[bufferIndex] = rawValue;
    bufferIndex = (bufferIndex + 1) % WINDOW_SIZE;
    if (bufferIndex == 0) bufferFilled = true;

    float temp[WINDOW_SIZE];
    for (int i = 0; i < WINDOW_SIZE; i++) temp[i] = buffer[i];

    for (int i = 0; i < WINDOW_SIZE - 1; i++) {
      for (int j = i + 1; j < WINDOW_SIZE; j++) {
        if (temp[i] > temp[j]) {
          float swap = temp[i];
          temp[i] = temp[j];
          temp[j] = swap;
        }
      }
    }
    float medianValue = bufferFilled ? temp[WINDOW_SIZE / 2] : rawValue;

    if (isFirstReading) {
      emaValue = medianValue;
      isFirstReading = false;
    } else {
      emaValue = (EMA_ALPHA * medianValue) + ((1.0f - EMA_ALPHA) * emaValue);
    }
    return emaValue;
  }
};

// Inisialisasi Filter
SensorFilter Filter_Suhu, Filter_Kelembapan, Filter_UV, Filter_Tekanan, Filter_Cahaya;

// Variabel Penampung Data Bersih
float Clean_Suhu, Clean_Kelembapan, Clean_UV, Clean_Tekanan, Clean_Cahaya;

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  Serial.print("\nConnecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32S3_WeatherStation")) {
      Serial.println("Connected!");
    } else {
      Serial.print("failed, rc="); Serial.print(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  while (Sensor_Lingkungan.begin() != 0) {
    Serial.println("Sensor tidak ditemukan!");
    delay(1000);
  }
  Serial.println("Sensor Ready!");

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long currentMillis = millis();

  // --- TAHAP 1: PEMBACAAN & FILTERING (Tiap 2 detik) ---
  if (currentMillis - Waktu_Terakhir_Baca >= Interval_Baca) {
    Waktu_Terakhir_Baca = currentMillis;

    Clean_Suhu = Filter_Suhu.process(Sensor_Lingkungan.getTemperature(TEMP_C));
    Clean_Kelembapan = Filter_Kelembapan.process(Sensor_Lingkungan.getHumidity());
    Clean_UV = Filter_UV.process(Sensor_Lingkungan.getUltravioletIntensity() + 7.77);
    if (Clean_UV < 0.0) {
      Clean_UV = 0.0;
    }
    Clean_Tekanan = Filter_Tekanan.process((float)Sensor_Lingkungan.getAtmospherePressure(HPA));
    Clean_Cahaya = Filter_Cahaya.process((float)Sensor_Lingkungan.getLuminousIntensity());
    //Clean_Elevasi = Filter_Elevasi.process(Sensor_Lingkungan.getElevation());

    Serial.print("Suhu       : "); Serial.print(Clean_Suhu); Serial.println(" C");
    Serial.print("Kelembapan : "); Serial.print(Clean_Kelembapan); Serial.println(" %");
    Serial.print("Indeks UV  : "); Serial.print(Clean_UV); Serial.println(" UVI");
    Serial.print("Intensitas Cahaya : "); Serial.print(Clean_Cahaya); Serial.println(" lux");
    Serial.print("Tekanan Udara : "); Serial.print(Clean_Tekanan); Serial.println(" hPa");
  }

  // --- TAHAP 2: PENGIRIMAN DATA KE IOT STACK (Tiap 30 detik) ---
  if (currentMillis - Waktu_Terakhir_Kirim >= Interval_Kirim) {
    Waktu_Terakhir_Kirim = currentMillis;

    StaticJsonDocument<256> doc;
    doc["suhu"] = Clean_Suhu;
    doc["kelembapan"] = Clean_Kelembapan;
    doc["cahaya"] = Clean_Cahaya;
    doc["uvi"] = Clean_UV;
    doc["tekanan"] = Clean_Tekanan;
    //doc["elevasi"] = Clean_Elevasi;

    char buffer[256];
    serializeJson(doc, buffer);

    if (client.publish("iot/stasiun_cuaca", buffer)) {
      Serial.println(">> Data Sent to MQTT Stack!");
    }
  }
}
