# ClariaSense: Automated System for Optimal Water Quality in Catfish Ponds

An IoT-powered aquarium monitoring and water replacement system using an ESP32 and Arduino Uno. Monitors water quality in real time and automatically drains, dumps, and refills water when parameters go out of safe range.

---

## 📦 Features

- Monitors **pH**, **TDS (ppm)**, **Temperature (°C)**, **Turbidity (NTU)**, and **flow rates**
- Automatically switches between 4 modes:
  - 🔁 Continuous Mode (normal flow)
  - 💧 Drain Mode (partial water removal)
  - 🗑️ Dump Mode (flushes dirty water)
  - 🚿 Fill Mode (adds clean water)
- Uses **OR logic** to trigger replacement based on:
  - TDS > `300 ppm`
  - pH < `6.0` or > `8.5`
  - Temp < `20°C` or > `32°C`
  - NTU < `20` or > `80`
- **Real-time volume tracking** (L) using flow meters
- Manual test commands via Serial for simulation
- Timer-based test mode (`flowless`) for bench testing

---

## 📐 Hardware Used

- **ESP32 (DevKit v1)**
- **Arduino Uno**
- **pH Sensor** (pH-4502C)
- **TDS Sensor**
- **DS18B20 Temperature Sensor**
- **Turbidity Sensor**
- **3x Flow Meters (YF-S201 or similar)**
- **4x Relays (Normally Closed)**
- 12V or 5V power supply
- Aquarium pump motors

---

## 📊 System Modes

| Mode        | Description                                   | Relays Active |
|-------------|-----------------------------------------------|----------------|
| Continuous  | Filters water in a loop                       | Relay 1 + 2    |
| Drain       | Removes 2–3 gallons when triggered            | Relay 1        |
| Dump        | Empties water to drain (time-based)           | Relay 3        |
| Fill        | Refills clean water to original volume        | Relay 4        |

---

## 🧪 Test Commands (Serial)

| Command                     | Description                            |
|-----------------------------|----------------------------------------|
| `test:7.2,350,25,85`        | Simulate high TDS (3 gal replacement)  |
| `test:5.5,250,25,50`        | Simulate low pH (2 gal replacement)    |
| `flowless`                  | Use timers instead of flow meters      |
| `flowon`                    | Re-enable real flow logic              |
| `reset`                     | Return to live sensor mode             |

---

## 💻 Upload Instructions

### Arduino UNO (flow + temp + turbidity sender):
Upload `arduino_sender.ino` to UNO. It:
- Counts pulses from 3 flow meters
- Reads turbidity + DS18B20 temp
- Sends JSON data to ESP32 via Serial

### ESP32:
Upload `esp32_main.ino` to your ESP32.
- Uses Serial2 (RX2=16, TX2=17) to receive data
- Controls relays and system logic

---
