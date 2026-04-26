# Stasiun Cuaca IoT (Low-End Server Edition)

Proyek monitoring lingkungan real-time yang dibangun di atas laptop "bekas" AMD E2-9000e. Membuktikan bahwa inovasi IoT tidak harus mahal!

## Fitur Utama
- **Monitoring 5 Parameter:** Suhu, Kelembapan, Cahaya (Lux), UV Index dan Tekanan Udara.
- **Cascading Filter:** Menggunakan kombinasi Median & EMA Filter untuk data sensor stabil dan minim noise.
- **Full Stack IoT:** ESP32-S3 -> MQTT (Mosquitto) -> Node-RED -> InfluxDB -> Grafana.
- **Server Mandiri:** Semua sistem berjalan di dalam Docker (Self-hosted).

## Perangkat & Teknologi
- **Hardware:** ESP32-S3, DFRobot Environmental Sensor (SEN0501).
- **Server:** Laptop HP AMD E2-9000e, RAM 4GB (Ubuntu 26.04 LTS + Docker).
- **Stack:** Mosquitto Broker, Node-RED, InfluxDB 1.8, Grafana.

## Skema Rangkaian
> [Masukkan Link Wokwi Mas di sini atau lampirkan gambar di folder schematics]
- **SDA:** Pin 8
- **SCL:** Pin 9

## Cara Instalasi Server (Docker)
Gunakan perintah berikut untuk menjalankan stack di laptop:
```bash
docker run -d --name influxdb -p 8086:8086 influxdb:1.8
docker run -d --name grafana -p 3000:3000 grafana/grafana
