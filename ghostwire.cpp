#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <esp_system.h>
#include <esp_chip_info.h>

// ============================================================
// PROJECT METADATA
// ============================================================
#define GHOSTWIRE_NAME    "GhostWire"
#define GHOSTWIRE_VERSION "1.0.0"
#define GHOSTWIRE_AUTHOR  "Mayank Malaviya aka AIwolfie"
#define GHOSTWIRE_TAGLINE "ESP32 Wireless Discovery & Device Diagnostics"
#define GHOSTWIRE_BOARD   "ESP32-WROOM"
#define GHOSTWIRE_YEAR    "2026"

// ============================================================
// FORWARD DECLARATIONS
// ============================================================
void printBanner();
void printMenu();
void handleChoice(int choice);
void wifiScan();
String wifiSecurity(wifi_auth_mode_t type);
void bleScan(int seconds);
void chipInfo();
void memFlashInfo();
void credits();
void systemStatus();
void initBLE();

// ============================================================
// BLE CALLBACK CLASS
// ============================================================
class GhostWireBLECallback : public BLEAdvertisedDeviceCallbacks {
private:
  int count = 0;

public:
  void resetCount() {
    count = 0;
  }

  int getCount() const {
    return count;
  }

  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    count++;
    Serial.println();
    Serial.printf("[%02d] %s\n", count, advertisedDevice.getAddress().toString().c_str());
    Serial.printf("     RSSI : %d dBm\n", advertisedDevice.getRSSI());
    if (advertisedDevice.haveName() && advertisedDevice.getName().length() > 0) {
      Serial.printf("     Name : %s\n", advertisedDevice.getName().c_str());
    } else {
      Serial.println("     Name : Unknown");
    }
    if (advertisedDevice.haveServiceUUID()) {
      Serial.printf("     UUID : %s\n", advertisedDevice.getServiceUUID().toString().c_str());
    }
  }
};

// ============================================================
// GLOBALS
// ============================================================
BLEScan* pBLEScan = nullptr;
static GhostWireBLECallback bleCallback;

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);
  delay(2000);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(200);
  printBanner();
  printMenu();
}

// ============================================================
// MAIN LOOP
// ============================================================
void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    // Ignore empty lines
    if (input.length() == 0) {
      return;
    }
    if (input.length() == 1 && input[0] >= '0' && input[0] <= '7') {
      int choice = input.toInt();
      handleChoice(choice);
    } else {
      Serial.println();
      Serial.println("[!] Invalid choice.");
      Serial.println("[*] Enter a number from 0 to 7.");
    }
    delay(300);
    printMenu();
  }
  delay(20);
}

// ============================================================
// BANNER
// ============================================================
void printBanner() {
  Serial.println();
  Serial.println("+------------------------------------------------------+");
  Serial.println("|                                                      |");
  Serial.println("|                    G H O S T W I R E                 |");
  Serial.println("|                                                      |");
  Serial.println("|    ESP32 Wireless Discovery & Device Diagnostics     |");
  Serial.println("|                                                      |");
  Serial.println("+------------------------------------------------------+");
  Serial.println("|  WiFi  |  BLE  |  Hardware  |  Diagnostics           |");
  Serial.println("+------------------------------------------------------+");
  Serial.println();
  Serial.println("  [ SYSTEM ONLINE ]");
  Serial.println();
  Serial.printf("  Device  : %s\n", GHOSTWIRE_BOARD);
  Serial.println("  Mode    : Standalone");
  Serial.println("  Serial  : 115200");
  Serial.printf("  Version : %s\n", GHOSTWIRE_VERSION);
  Serial.println();
  Serial.printf("  Created by: %s\n", GHOSTWIRE_AUTHOR);
}

// ============================================================
// MENU
// ============================================================
void printMenu() {
  Serial.println();
  Serial.println("--------------------------------------------------------");
  Serial.println("                     MAIN MENU");
  Serial.println("--------------------------------------------------------");
  Serial.println();
  Serial.println("  WIRELESS");
  Serial.println("    [1] WiFi Network Scan");
  Serial.println();
  Serial.println("  BLUETOOTH");
  Serial.println("    [2] BLE Scan - 10 seconds");
  Serial.println("    [3] BLE Scan - 30 seconds");
  Serial.println();
  Serial.println("  DEVICE");
  Serial.println("    [4] Chip Information");
  Serial.println("    [5] Memory / Flash Information");
  Serial.println();
  Serial.println("  SYSTEM");
  Serial.println("    [6] Credits / About");
  Serial.println("    [7] System Status");
  Serial.println("    [0] Show Banner");
  Serial.println();
  Serial.println("--------------------------------------------------------");
  Serial.print("  GhostWire > Enter choice: ");
}

// ============================================================
// MENU HANDLER
// ============================================================
void handleChoice(int choice) {
  switch (choice) {
    case 1:
      wifiScan();
      break;
    case 2:
      bleScan(10);
      break;
    case 3:
      bleScan(30);
      break;
    case 4:
      chipInfo();
      break;
    case 5:
      memFlashInfo();
      break;
    case 6:
      credits();
      break;
    case 7:
      systemStatus();
      break;
    case 0:
      printBanner();
      break;
    default:
      Serial.println();
      Serial.println("[!] Invalid choice.");
      Serial.println("[*] Enter a number from 0 to 7.");
      break;
  }
}

// ============================================================
// WIFI SCANNER
// ============================================================
void wifiScan() {
  Serial.println();
  Serial.println("+------------------------------------------------------+");
  Serial.println("|                 WIFI NETWORK SCAN                    |");
  Serial.println("+------------------------------------------------------+");
  Serial.println();
  Serial.println("[*] Initializing WiFi radio...");
  Serial.println("[*] Scanning 2.4 GHz channels...");

  WiFi.scanDelete();
  int networks = WiFi.scanNetworks(
    false,   // async
    false,   // show hidden
    false,   // passive
    300,     // max milliseconds per channel
    0        // channel
  );

  if (networks < 0) {
    Serial.println();
    Serial.println("[!] WiFi scan failed.");
    return;
  }
  if (networks == 0) {
    Serial.println();
    Serial.println("[-] No WiFi networks found.");
    WiFi.scanDelete();
    return;
  }

  Serial.println();
  Serial.println("[+] Scan complete");
  Serial.printf("[+] Networks found: %d\n", networks);
  Serial.println();
  Serial.println(" #   SSID                         CH     RSSI     SECURITY");
  Serial.println("------------------------------------------------------------");

  for (int i = 0; i < networks; i++) {
    String ssid = WiFi.SSID(i);
    int32_t rssi = WiFi.RSSI(i);
    int32_t channel = WiFi.channel(i);
    wifi_auth_mode_t encryption = WiFi.encryptionType(i);

    if (ssid.length() == 0) {
      ssid = "<hidden>";
    }
    if (ssid.length() > 28) {
      ssid = ssid.substring(0, 25) + "...";
    }

    char rssiStr[12];
    snprintf(rssiStr, sizeof(rssiStr), "%d dBm", rssi);

    Serial.printf(
      "%2d   %-28s %-6d %-9s %s\n",
      i + 1,
      ssid.c_str(),
      channel,
      rssiStr,
      wifiSecurity(encryption).c_str()
    );
  }

  Serial.println("------------------------------------------------------------");
  WiFi.scanDelete();
  Serial.println();
  Serial.println("[+] WiFi scan complete.");
}

// ============================================================
// WIFI SECURITY
// ============================================================
String wifiSecurity(wifi_auth_mode_t type) {
  switch (type) {
    case WIFI_AUTH_OPEN:
      return "OPEN";
    case WIFI_AUTH_WEP:
      return "WEP";
    case WIFI_AUTH_WPA_PSK:
      return "WPA";
    case WIFI_AUTH_WPA2_PSK:
      return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE:
      return "WPA2-ENT";
    case WIFI_AUTH_WPA3_PSK:
      return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:
      return "WPA2/WPA3";
    default:
      return "UNKNOWN";
  }
}

// ============================================================
// BLE INITIALIZATION
// ============================================================
void initBLE() {
  if (pBLEScan == nullptr) {
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    if (pBLEScan == nullptr) {
      return;
    }
    pBLEScan->setAdvertisedDeviceCallbacks(&bleCallback, true);
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
  }
}

// ============================================================
// BLE SCANNER
// ============================================================
void bleScan(int seconds) {
  Serial.println();
  Serial.println("+------------------------------------------------------+");
  Serial.println("|                   BLE DISCOVERY                      |");
  Serial.println("+------------------------------------------------------+");
  Serial.println();
  Serial.printf("[*] Scan duration : %d seconds\n", seconds);
  Serial.println("[*] BLE scanner   : Active");
  Serial.println("[*] Listening for advertisements...");

  initBLE();
  if (pBLEScan == nullptr) {
    Serial.println();
    Serial.println("[!] Failed to initialize BLE scanner.");
    return;
  }

  bleCallback.resetCount();
  BLEScanResults results = pBLEScan->start(seconds, false);

  Serial.println();
  Serial.println("--------------------------------------------------------");
  Serial.println("[+] BLE scan complete");
  Serial.printf("[+] Devices discovered: %d\n", results.getCount());

  pBLEScan->clearResults();
}

// ============================================================
// CHIP INFORMATION
// ============================================================
void chipInfo() {
  esp_chip_info_t info;
  esp_chip_info(&info);

  Serial.println();
  Serial.println("+------------------------------------------------------+");
  Serial.println("|                 CHIP INFORMATION                     |");
  Serial.println("+------------------------------------------------------+");
  Serial.println();
  Serial.print("Model          : ");
  switch (info.model) {
    case CHIP_ESP32:
      Serial.println("ESP32");
      break;
    case CHIP_ESP32S2:
      Serial.println("ESP32-S2");
      break;
    case CHIP_ESP32S3:
      Serial.println("ESP32-S3");
      break;
    case CHIP_ESP32C3:
      Serial.println("ESP32-C3");
      break;
#if defined(CHIP_ESP32C6)
    case CHIP_ESP32C6:
      Serial.println("ESP32-C6");
      break;
#endif
#if defined(CHIP_ESP32H2)
    case CHIP_ESP32H2:
      Serial.println("ESP32-H2");
      break;
#endif
    default:
      Serial.println("Other ESP32 family");
      break;
  }
  Serial.printf("Revision       : %d\n", info.revision);
  Serial.printf("CPU Cores      : %d\n", info.cores);
  Serial.printf("CPU Frequency  : %u MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("SDK Version    : %s\n", ESP.getSdkVersion());
  Serial.printf("MAC Address    : %s\n", WiFi.macAddress().c_str());
  Serial.println();
  Serial.printf("WiFi           : %s\n", (info.features & CHIP_FEATURE_WIFI_BGN) ? "Available" : "Not available");
  Serial.printf("Bluetooth      : %s\n", (info.features & CHIP_FEATURE_BT) ? "Available" : "Not available");
  Serial.printf("BLE            : %s\n", (info.features & CHIP_FEATURE_BLE) ? "Available" : "Not available");
  Serial.printf("Embedded Flash : %s\n", (info.features & CHIP_FEATURE_EMB_FLASH) ? "Available" : "Not available");
}

// ============================================================
// MEMORY / FLASH INFORMATION
// ============================================================
void memFlashInfo() {
  Serial.println();
  Serial.println("+------------------------------------------------------+");
  Serial.println("|              MEMORY / FLASH STATUS                   |");
  Serial.println("+------------------------------------------------------+");
  Serial.println();
  Serial.printf("Heap Total          : %u KB\n", ESP.getHeapSize() / 1024);
  Serial.printf("Heap Free           : %u KB\n", ESP.getFreeHeap() / 1024);
  Serial.printf("Minimum Heap        : %u KB\n", ESP.getMinFreeHeap() / 1024);
  Serial.printf("Largest Free Block  : %u KB\n", ESP.getMaxAllocHeap() / 1024);
  Serial.println();
  if (psramFound() && ESP.getPsramSize() > 0) {
    Serial.printf("PSRAM Total         : %u KB\n", ESP.getPsramSize() / 1024);
    Serial.printf("PSRAM Free          : %u KB\n", ESP.getFreePsram() / 1024);
  } else {
    Serial.println("PSRAM Total         : Not available");
    Serial.println("PSRAM Free          : Not available");
  }
  Serial.println();
  Serial.printf("Flash Size          : %u MB\n", ESP.getFlashChipSize() / (1024 * 1024));
  Serial.printf("Flash Speed         : %u MHz\n", ESP.getFlashChipSpeed() / 1000000);
  Serial.println();
  Serial.printf("Sketch Size         : %u KB\n", ESP.getSketchSize() / 1024);
  Serial.printf("Free Sketch Space   : %u KB\n", ESP.getFreeSketchSpace() / 1024);
  Serial.println();
  Serial.printf("Uptime              : %lu seconds\n", millis() / 1000);
}

// ============================================================
// CREDITS / ABOUT
// ============================================================
void credits() {
  Serial.println();
  Serial.println("+------------------------------------------------------+");
  Serial.println("|                    ABOUT GHOSTWIRE                   |");
  Serial.println("+------------------------------------------------------+");
  Serial.println();
  Serial.printf("%s v%s\n", GHOSTWIRE_NAME, GHOSTWIRE_VERSION);
  Serial.println();
  Serial.println(GHOSTWIRE_TAGLINE);
  Serial.println();
  Serial.printf("Created by : %s\n", GHOSTWIRE_AUTHOR);
  Serial.println("Platform   : ESP32");
  Serial.printf("Board      : %s\n", GHOSTWIRE_BOARD);
  Serial.println("Framework  : Arduino");
  Serial.println();
  Serial.println("--------------------------------------------------------");
  Serial.println();
  Serial.println("GhostWire is an educational embedded utility for");
  Serial.println("wireless discovery and ESP32 hardware diagnostics.");
  Serial.println();
  Serial.println("Use responsibly and only on networks/devices you");
  Serial.println("are authorized to inspect.");
  Serial.println();
  Serial.println("--------------------------------------------------------");
  Serial.println();
  Serial.printf("(c) %s %s\n", GHOSTWIRE_YEAR, GHOSTWIRE_AUTHOR);
}

// ============================================================
// SYSTEM STATUS
// ============================================================
void systemStatus() {
  unsigned long totalSeconds = millis() / 1000;
  unsigned int hours = totalSeconds / 3600;
  unsigned int minutes = (totalSeconds % 3600) / 60;
  unsigned int seconds = totalSeconds % 60;

  Serial.println();
  Serial.println("+------------------------------------------------------+");
  Serial.println("|                   SYSTEM STATUS                      |");
  Serial.println("+------------------------------------------------------+");
  Serial.println();
  Serial.printf("Project       : %s\n", GHOSTWIRE_NAME);
  Serial.printf("Version       : %s\n", GHOSTWIRE_VERSION);
  Serial.printf("Author        : %s\n", GHOSTWIRE_AUTHOR);
  Serial.printf("Platform      : %s\n", GHOSTWIRE_BOARD);
  Serial.println();
  Serial.printf("WiFi          : %s\n", (WiFi.getMode() != WIFI_OFF) ? "Ready" : "Offline");
  Serial.println("BLE           : Ready");
  Serial.println();
  Serial.printf("Uptime        : %02u:%02u:%02u\n", hours, minutes, seconds);
  Serial.printf("Free Heap     : %u KB\n", ESP.getFreeHeap() / 1024);
  Serial.printf("Flash         : %u MB\n", ESP.getFlashChipSize() / (1024 * 1024));
}