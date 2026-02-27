# M5Stack-WebCam

A WebCam for the M5Stack CoreS3 with browser streaming, face recognition triggered recording to SD.

![M5Stack-WebCam](preview.png)

## ✨ Features

* **Face Detection:**
  Real-time face detection using the built-in ESP32-S3 camera capabilities. When a face is detected, optional automatic recording can be triggered. Recording continues for a configurable timeout (`face_timeout_s`) after the last detected face. Ideal for motion-based or presence-based capture scenarios.

* **Record To SD:**
  Automatically records video (or image sequences depending on configuration) directly to the MicroSD card in FAT32 format. Recording can be triggered by face detection or controlled via the web interface. A visual recording indicator can be shown on the device display when `record_icon` is enabled.

* **Browser File Access:**
  Built-in web-based file manager (ESP File Manager) allows you to browse, download, and delete recorded files directly from your browser. No need to remove the SD card. Accessible from any device on the same network.

* **Browser Live Feed:**
  Live camera stream accessible via a web browser. View the real-time feed from your desktop, tablet, or phone. No additional software required — just open the device IP address in your browser.

---

## 📦 Software & Libraries

This project is built using **PlatformIO**. The following libraries are required and are defined in the `lib_deps` section of `platformio.ini`.

### Required Dependencies

1. **M5Unified** (by M5Stack)
   Unified hardware abstraction library for M5Stack devices.
   PlatformIO: `m5stack/M5Unified`

2. **ESP File Manager** (by arslan437)
   Web-based file manager for ESP32 (SPIFFS/LittleFS support).
   GitHub: https://github.com/arslan437/EspFileManager.git

3. **AsyncTCP** (by ESP32Async)
   Asynchronous TCP library for ESP32. Required by ESPAsyncWebServer.
   PlatformIO: `ESP32Async/AsyncTCP`

4. **ESPAsyncWebServer**
   Asynchronous web server library for ESP32.

5. **ArduinoJson** (by Benoît Blanchon)
   JSON parsing and serialization library for embedded systems.
   PlatformIO: `ArduinoJson`

6. **DNSServer**
   DNS server library (commonly used for captive portal implementations).

> **Note:** AviWriter is adapted from https://github.com/s60sc/ESP32-CAM_MJPEG2SD
---

## 🚀 Installation

### 1️⃣ Prepare a `config.json` File

Create a file named `config.json` with the following content:

```json
{
    "ssid": "<your-ssid>",
    "password": "<your-password>",
    "record": true,
    "record_icon": true,
    "show_camera": false,
    "face_timeout_s": 10
}
```

**Where:**

- `ssid`, `password` — Your WiFi credentials.
  Leave both empty (`""`) to run the camera without WiFi.
- `record` — Set to `true` to record when a face is detected.
- `record_icon` — Displays a recording indicator on the screen.
- `show_camera` — Mirrors the live camera feed to the device display.
- `face_timeout_s` — Number of seconds to continue recording after face detection stops.

---

### 2️⃣ Prepare the SD Card

- Format your MicroSD card as **FAT32**.
- Copy `config.json` onto the SD card.
- Insert the SD card into the **Core S3**.
- Ensure the SD card is properly seated before powering on the device.

---

### 3️⃣ Setup PlatformIO IDE

1. Install **Visual Studio Code**
   https://code.visualstudio.com/

2. Install the **PlatformIO IDE extension** from the VS Code Extensions marketplace.

3. Clone this repository:

   ```bash
   git clone https://github.com/griffithsb/M5Stack-WebCam/M5Stack-WebCam.git
   ```

4. Open the project folder in VS Code.

5. PlatformIO will automatically detect the `platformio.ini` file and install all required dependencies defined under `lib_deps`.

6. Wait for dependency installation to complete before building.

---

### 4️⃣ Flash the Code

#### Flash using PlatformIO

1. Connect your **M5Stack Core S3** to your computer via USB-C.
2. In VS Code, open the PlatformIO sidebar.
3. Click **Build** to compile the firmware.
4. Click **Upload** to flash the device.
5. Open the **Serial Monitor** (115200 baud) to view logs and confirm successful startup.

#### Standalone Flash (Precompiled Binary)

If using a precompiled `.bin` file:

1. Download the firmware binary.
2. Use **esptool**:

   ```bash
   esptool.py --chip esp32s3 --port <your-port> --baud 921600 write_flash 0x0 firmware.bin
   ```

   Replace `<your-port>` with your device port (e.g., `COM3` or `/dev/ttyUSB0`).

3. Reset the device after flashing.

---

## 🌐 Accessing the Web Interface

Once powered on:

- If WiFi credentials are provided, the device connects to your network.
- Check your router for the assigned IP address.
- Open a browser and navigate to:

```
http://<device-ip>
```

to access the live stream, or:

```
http://<device-ip>/file
```

to manage files on the SD card

If WiFi credentials are empty, the device will run in standalone mode (no web interface).

---

## 📜 License

MIT License

---



