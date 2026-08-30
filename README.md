# GhostWire

**ESP32 Wireless Discovery & Device Diagnostics**

A compact wireless discovery and hardware diagnostics toolkit for ESP32.

![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32-green.svg)
![Framework](https://img.shields.io/badge/framework-Arduino-teal.svg)
![Serial](https://img.shields.io/badge/serial-115200-orange.svg)
![License](https://img.shields.io/badge/license-MIT-lightgrey.svg)

---

## What is GhostWire?

GhostWire turns a standard ESP32 development board into a **wireless discovery and diagnostics terminal** — no display, no external shield, no dashboard. Just an ESP32, a USB cable, and a serial monitor.

The menu-driven interface provides:

| Capability | Status |
|---|---|
| 📡 Wi-Fi Discovery | ✅ |
| 🟦 BLE Discovery | ✅ |
| 🧠 ESP32 Diagnostics | ✅ |
| 💾 Memory & Flash Diagnostics | ✅ |
| 🖥️ Serial Interface | ✅ |

---

## Interface Preview

```
+------------------------------------------------------+
|                                                      |
|                    G H O S T W I R E                 |
|                                                      |
|    ESP32 Wireless Discovery & Device Diagnostics     |
|                                                      |
+------------------------------------------------------+
|  WiFi  |  BLE  |  Hardware  |  Diagnostics           |
+------------------------------------------------------+

  [ SYSTEM ONLINE ]

  Device  : ESP32-WROOM
  Mode    : Standalone
  Serial  : 115200
  Version : 1.0.0

  Created by: Mayank Malaviya aka AIwolfie
```

---

## Features

### 📡 Wi-Fi Discovery

Scans 2.4 GHz channels and reports:

- **SSID** — network name (truncated if longer than 28 characters; hidden networks shown as `<hidden>`)
- **Channel** — 2.4 GHz channel number
- **RSSI** — signal strength in dBm
- **Security** — `OPEN`, `WEP`, `WPA`, `WPA2`, `WPA/WPA2`, `WPA2-ENT`, `WPA3`, `WPA2/WPA3`

```
 #   SSID                         CH     RSSI     SECURITY
------------------------------------------------------------
 1   HomeNetwork                  6     -48 dBm   WPA2
 2   Office                       11    -67 dBm   WPA2
 3   <hidden>                     1     -81 dBm   OPEN
------------------------------------------------------------
```

RSSI is affected by walls, antennas, interference, and environmental conditions. Do not treat it as an exact distance measurement.

### 🟦 BLE Discovery

Performs active BLE scans for nearby advertisements. For each device discovered, reports:

- **Bluetooth address** (MAC)
- **RSSI** in dBm
- **Advertised local name** (where available)
- **Service UUID** (where advertised)

```
[01] AA:BB:CC:DD:EE:FF
     RSSI : -54 dBm
     Name : DeviceName
     UUID : ...
```

GhostWire does **not** connect to, pair with, or interact with discovered devices. It only listens for advertisements.

### 🧠 Hardware Diagnostics

Reads ESP32 chip information directly from the hardware:

- Chip model, revision, CPU cores, CPU frequency
- ESP-IDF / SDK version
- MAC address
- Wi-Fi, Bluetooth, BLE, and embedded flash capability

### 💾 Memory & Flash Diagnostics

Reports runtime resource information:

- **Heap** — total, free, minimum free, largest free block
- **PSRAM** — total and free (detects presence; displays "Not available" when absent)
- **Flash** — size, speed, sketch size, free sketch space
- **Uptime** — seconds since boot

### 🖥️ Serial Interface

```
  WIRELESS
    [1] WiFi Network Scan

  BLUETOOTH
    [2] BLE Scan - 10 seconds
    [3] BLE Scan - 30 seconds

  DEVICE
    [4] Chip Information
    [5] Memory / Flash Information

  SYSTEM
    [6] Credits / About
    [7] System Status
    [0] Show Banner
```

---

## Hardware Requirements

- **ESP32-WROOM** or ESP32 Dev Module / compatible development board
- USB data cable
- Windows, Linux, or macOS computer

ESP32-WROOM modules commonly ship with **4 MB flash**. GhostWire's combined Wi-Fi + BLE firmware is approximately **1.52 MB**.

---

## Software Requirements

- **PlatformIO** (recommended) or **Arduino IDE 2.x**
- ESP32 Arduino board package (Espressif Systems)

---

## Installation

### PlatformIO

```bash
git clone https://github.com/AIwolfie/GhostWire.git
cd GhostWire
pio run
pio run -t upload
pio device monitor -b 115200
```

The provided `platformio.ini` uses `board_build.partitions = huge_app.csv` to ensure sufficient application space for the combined Wi-Fi + BLE firmware.

### Arduino IDE

#### Step 1 — Install Arduino IDE

Download and install **Arduino IDE 2.x** from the official website.

#### Step 2 — Install ESP32 Board Package

Open Arduino IDE and navigate to:

```
Tools → Board → Boards Manager
```

Search for `esp32` and install **esp32 by Espressif Systems**.

#### Step 3 — Open GhostWire

Clone or download the repository. Open `ghostwire.cpp` in Arduino IDE.

If Arduino IDE prompts for a sketch folder, create a folder named `GhostWire/` containing a file named `GhostWire.ino`, then copy the contents of `ghostwire.cpp` into it.

#### Step 4 — Select Board

```
Tools → Board → ESP32 Arduino → ESP32 Dev Module
```

For other ESP32 boards, select the specific board variant.

#### Step 5 — Select COM Port

```
Tools → Port
```

Select the COM port your ESP32 is connected to. On Windows this will appear as `COM3`, `COM5`, etc.

#### Step 6 — Configure Partition Scheme

```
Tools → Partition Scheme → Huge APP
```

GhostWire combines Wi-Fi + BLE in a single firmware image. The default partition scheme may not provide enough application space. Select **Huge APP** (3 MB application partition).

#### Step 7 — Compile

Click **Verify** (✓) or navigate to `Sketch → Verify/Compile`.

#### Step 8 — Upload

Click **Upload** (→). If your board requires bootloader mode, hold the **BOOT** button during upload.

#### Step 9 — Serial Monitor

Open `Tools → Serial Monitor`. Set baud rate to `115200` and line ending to `Newline` or `Both NL & CR`.

Press the ESP32 reset button if the startup banner does not appear. You should see the GhostWire banner and main menu.

---

## Usage

Enter a menu number in the Serial Monitor:

| Key | Function |
|---|---|
| `1` | Wi-Fi Network Scan |
| `2` | BLE Scan — 10 seconds |
| `3` | BLE Scan — 30 seconds |
| `4` | Chip Information |
| `5` | Memory / Flash Information |
| `6` | Credits / About |
| `7` | System Status |
| `0` | Show Banner |

---

## Build Verification

GhostWire v1.0.0 builds successfully with PlatformIO for the `esp32dev` target:

```
RAM:   [==        ]  17.2% (used 56276 bytes from 327680 bytes)
Flash: [=====     ]  48.3% (used 1519653 bytes from 3145728 bytes)
```

Actual values vary depending on ESP32 Arduino core version, compiler version, board configuration, partition scheme, and enabled features.

---

## Responsible Use

GhostWire is designed for:

- Embedded development and education
- Wireless environment discovery
- ESP32 experimentation
- Hardware diagnostics
- Testing networks and devices you own or are authorized to inspect

GhostWire intentionally does **not** provide:

- Wi-Fi password cracking or handshake capture
- Deauthentication or Evil Twin attacks
- Credential harvesting
- Packet injection or radio jamming
- BLE exploitation or pairing bypasses
- Unauthorized access mechanisms

**Discover responsibly. Scan only environments and devices you are authorized to inspect.**

---

## Roadmap

GhostWire is still evolving. Potential future improvements include:

- [ ] Configurable Wi-Fi scan settings
- [ ] Configurable scan duration
- [ ] CSV / JSON scan output
- [ ] Extended BLE advertisement parsing
- [ ] Manufacturer identification
- [ ] Additional ESP32 board diagnostics
- [ ] Automated GitHub Actions builds
- [ ] Release-ready firmware artifacts

Roadmap items are ideas, not promises or existing functionality.

---

## Contributing

Contributions are welcome.

### Good areas for contribution

- Bug fixes and memory optimizations
- ESP32 board compatibility
- Documentation improvements
- BLE parsing improvements
- Wi-Fi scan improvements
- Serial UI improvements
- Build automation
- Testing on physical hardware

### Workflow

```bash
# Fork the repository, then clone your fork
git clone https://github.com/AIwolfie/GhostWire.git
cd GhostWire

# Create a feature branch
git checkout -b feature/my-improvement

# Make your changes, then test
pio run

# Commit and push
git add .
git commit -m "Describe your improvement"
git push origin feature/my-improvement
```

Then open a Pull Request. Test on physical ESP32 hardware whenever possible.

### Scope

GhostWire remains focused on **wireless discovery, embedded diagnostics, and education**. Contributions introducing offensive functionality, unauthorized access mechanisms, or exploitation are outside the project's scope.

---

## Development

```bash
# Build
pio run

# Upload
pio run -t upload

# Serial monitor
pio device monitor -b 115200

# Clean build artifacts
pio run -t clean
```

---

## Project Structure

```
GhostWire/
├── ghostwire.cpp       # Main ESP32 application source
├── platformio.ini      # PlatformIO configuration
├── README.md           # This file
├── LICENSE             # MIT License
└── .gitignore          # Build artifact exclusions
```

---

## License

Licensed under the **MIT License**. See [LICENSE](LICENSE) for details.

Copyright © 2026 Mayank Malaviya aka AIwolfie

---

## Author

**Mayank Malaviya aka AIwolfie** — Creator and maintainer of GhostWire.

---

<div align="center">

**GhostWire** — ESP32 Wireless Discovery & Device Diagnostics

Mayank Malaviya aka AIwolfie · 2026

</div>