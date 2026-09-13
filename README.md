# WLED Video Streamer

An ultra-low latency, hardware-accelerated video streaming application that captures real-time video using an ESP32-S3 Camera board and streams it directly to a WLED-powered LED Matrix over Wi-Fi using the DDP protocol.

## 🚀 Features
* **Dual-Core Processing:** Captures and processes image data on Core 1 while asynchronously streaming UDP and WebSocket data on Core 0 for massive framerate stability.
* **Computer Vision Processing:** Real-time software contrast, saturation, unsharp masking, and Laplacian edge detection.
* **Video Effects:** Normal, Psychedelic, Retro 8-Bit, Cyberpunk, Thermal Camera, Digital Matrix, and Neon Edge Glow.
* **"Magic Mirror" Mode:** Captures the background of an empty room and subtracts it from the live feed, isolating human silhouettes. Includes motion smear/trails!
* **Web UI Configuration:** Fully configurable via a smartphone or web browser (No hardcoding required).
* **Mesh Network Optimized:** Automatically locks onto the strongest access point in a mesh Wi-Fi network.

## 🛠️ Hardware Requirements
* **ESP32-S3 Camera Board:** Must have PSRAM (e.g., Freenove ESP32-S3 WROOM CAM).
* **Camera Module:** OV3660 (or OV7725 for superior low-light performance).
* **WLED Matrix:** An LED panel driven by a separate ESP32 running WLED (e.g., 72x40 resolution).
* **(Optional) Hardware Interfaces:**
  * **Effect Cycle Button:** Momentary push button on GPIO 14 (Connects to GND).
  * **Stream Enable Switch:** Toggle switch on GPIO 13 (Connects to GND).
  * **Hardware Brightness:** LDR / Potentiometer on GPIO 4.

## 💻 How to Build and Flash

This project is built using [PlatformIO](https://platformio.org/).

1. **Install VS Code and PlatformIO:** Download VS Code and install the "PlatformIO IDE" extension.
2. **Open the Project:** Open the `WLED_Vid_Streamer` folder in VS Code.
3. **Connect your ESP32-S3:** Plug the board into your computer via USB. *(Note: Make sure your cable supports data transfer!)*
4. **Build and Upload:** 
   * Click the **PlatformIO icon** (alien head) on the left sidebar.
   * Expand your board environment (`esp32s3camlcd`).
   * Click **Upload**.
   * PlatformIO will automatically download all required libraries (WiFiManager, ESPAsyncWebServer, etc.), compile the C++ code, and flash the ESP32.

## ⚙️ Initial Setup & Usage

### 1. Wi-Fi Setup (Captive Portal)
When the ESP32 boots for the very first time, it won't know your home Wi-Fi password.
* Take out your smartphone and go to your Wi-Fi settings.
* Connect to the newly broadcasted network called **`WLED_Streamer_AP`**.
* A page will automatically pop up. Select your home Wi-Fi network and enter the password. The ESP32 will reboot and connect to your home network.

### 2. Access the Web Dashboard
* Ensure your phone/computer is on the same Wi-Fi network.
* Open a web browser and navigate to: **`http://wled-vid-streamer.local`**
*(If this doesn't work, look up the IP address of the ESP32 in your router's DHCP list and type that into the browser).*

### 3. Connect to WLED
On the web dashboard:
1. Enter the **WLED IP Address** of your LED matrix.
2. Enter your matrix dimensions (e.g., Width: 72, Height: 40).
3. Check the **"Enable Video Stream"** box.
4. Click **Save Settings**.
Your LED matrix should immediately begin mirroring the camera feed!

### 4. Tuning for Performance
* **Target Framerate:** 10 to 15 FPS is recommended for stability.
* **Web UI Live Preview:** Uncheck this toggle when you are done configuring! The live browser preview consumes massive amounts of Wi-Fi bandwidth. Turning it off dedicates 100% of the ESP32's antenna to the WLED matrix, resulting in a significantly smoother LED framerate.

## 🕹️ Interacting with the Display

* **Attract Mode:** If no one presses the physical hardware button for a set amount of time, the display will automatically cycle through video effects. You can configure which effects are included in this rotation via the checklist at the bottom of the Web UI.
* **Magic Mirror Calibration:** Clear the kids out of the room, click the **"Capture Empty Room"** button on the Web UI, and enable Background Subtraction. The room will vanish, leaving only people floating in pure blackness!

