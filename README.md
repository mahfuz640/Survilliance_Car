# 🚀 ESP32 Robot Control System with Camera

This project is an **ESP32-based Robot Control & Monitoring System** with the following features:

- ✅ **Wi-Fi Access Point** – ESP32 runs as a hotspot, and hosts a control webpage.  
- ✅ **Servo Control** – Three servo motors can be controlled (open/close, up/down, short plate).  
- ✅ **DC Motor Car Control** – Forward, backward, left, right movement with **hold-to-move buttons**.  
- ✅ **Metal Detector Sensor** – Detects metal vs non-metal and updates in **real-time** (every 100ms).  
- ✅ **DHT Sensor** – Temperature and humidity monitoring.  
- ✅ **Digital Compass** – Live compass heading visualization with direction (N/E/S/W).  
- ✅ **ESP32-CAM Integration** – Live video stream directly on the web UI.  
- ✅ **Colorful Web UI** – Gradient buttons, non-selectable text, and smooth status updates.  

---

## 🖥️ Web UI Preview
- Color-filled buttons (hold for continuous control).  
- **Metal Detector status** → Green (Non-Metal) / Red (Metal).  
- **Compass Dial** → Live arrow rotation with heading text.  
- **DHT Sensor Values** → Auto-refresh every 0.1s.  
- **ESP32-CAM Feed** → Embedded video window on webpage.  

---

## 📡 How It Works
1. ESP32 creates a Wi-Fi Access Point (`ESP32_AP`, password: `12345678`).  
2. User connects via phone/PC and opens ESP32’s webpage.  
3. Control interface loads:  
   - Servo buttons control gripper/plate servos.  
   - Car buttons allow directional control (hold = move, release = stop).  
   - Metal sensor + DHT sensor data auto-updates every **0.1s**.  
   - Compass heading displayed on a live canvas dial.  
   - **ESP32-CAM stream is embedded in the same page**.  

---

## ⚙️ Hardware Required
- ESP32 Dev Board  
- ESP32-CAM Module  
- 3 × Servo Motors  
- 4 × DC Motors + Motor Driver (L298N / L293D)  
- DHT11 / DHT22 Sensor  
- Metal Detector Sensor (digital output)  
- Compass Sensor (HMC5883L / QMC5883L via I²C)  
- Power Supply (Battery Pack)  

---

## 🔌 Pin Configuration (Example)
| Component       | ESP32 Pin |
|-----------------|-----------|
| Servo 1         | GPIO 14   |
| Servo 2         | GPIO 12   |
| Servo 3         | GPIO 13   |
| Motor IN1       | GPIO 12   |
| Motor IN2       | GPIO 13   |
| Motor IN3       | GPIO 14   |
| Motor IN4       | GPIO 27   |
| Rotor IN3       | GPIO 25   |
| Rotor IN4       | GPIO 33   |
| Plow IN1        | GPIO 18   |
| Plow IN2        | GPIO 19   |
| Metal Sensor    | GPIO xx   |
| DHT Sensor      | GPIO xx   |
| Compass (SDA)   | GPIO 21   |
| Compass (SCL)   | GPIO 22   |
| ESP32-CAM       | GPIO default (OV2640 pins) |

*(adjust pin numbers based on your wiring)*  

---

## 🛠️ Installation
1. Install **Arduino IDE** with ESP32 board support.  
2. Install required libraries:  
   - `ESP32Servo`  
   - `DHT`  
   - `Wire`  
   - `esp32-camera` (for ESP32-CAM stream)  
3. Upload the code to ESP32 + ESP32-CAM.  
4. Connect to `ESP32_AP` Wi-Fi.  
5. Open browser → `192.168.4.1` → control your robot & see live video feed!  

---

## 🎨 Features in Action
- Car can be driven with on-screen hold buttons.  
- Servo positions update instantly.  
- Metal detection instantly changes status color.  
- Compass arrow rotates in real-time.  
- Live temperature & humidity monitoring.  
- **Live ESP32-CAM video** directly on webpage.  

---

## 📷 Demo Screenshot
*(Add screenshots of Web UI with Camera stream here)*  

---

## 📜 License  

---

👉 Ready to use, just flash the code and control your robot wirelessly — now with live camera feed! 🎥🚗⚙️  
