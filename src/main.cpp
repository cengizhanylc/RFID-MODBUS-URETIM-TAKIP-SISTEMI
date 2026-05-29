#include <Arduino.h>
#include <WiFi.h>
#include <nvs_flash.h>

#include "Config.h"
#include "ModbusServer.h"
#include "NvsManager.h"
#include "RfidReader.h"
#include "WebServerManager.h"
#include "WifiManager.h"

// --- GLOBAL POINTERLAR ---
NvsManager* gNvsManager = nullptr;
WifiManager* gWifiManager = nullptr;
WebServerManager* gWebServerManager = nullptr;
RfidReader* gRfidReader = nullptr;
ModbusServer* gModbusServer = nullptr;

unsigned long lastRfidCheckMs = 0;
unsigned long lastUidReadTime = 0; 

bool initEspNvsFlash() {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_OK) return true;
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    if (nvs_flash_erase() != ESP_OK) return false;
    err = nvs_flash_init();
    return err == ESP_OK;
  }
  return false;
}

void setup() {
  delay(2000); 
  Serial.begin(115200);
  Serial.println("\n[BOOT] TeosSensor starting...");

  if (!initEspNvsFlash()) Serial.println("[BOOT] Failed to initialize NVS.");

  gNvsManager = new NvsManager();
  gWifiManager = new WifiManager(*gNvsManager);
  gWebServerManager = new WebServerManager(*gNvsManager);
  gRfidReader = new RfidReader();
  gModbusServer = new ModbusServer();

  if (!gNvsManager->begin()) Serial.println("[BOOT] NVS init failed.");
  if (!gRfidReader->begin()) Serial.println("[BOOT] RFID init failed.");
  if (!gWifiManager->begin()) Serial.println("[BOOT] Wi-Fi init failed.");
  
  gWebServerManager->begin();

  if (gWifiManager->isStaMode()) {
    if (!gModbusServer->begin()) Serial.println("[BOOT] Modbus init failed.");
  }

  // Cihaz STA modunda açıldığında, önceki oturumdan kalan son UID'yi geri yükle (Web Paneli için)
  uint32_t lastUid = 0;
  if (gNvsManager->getLastUid(lastUid)) {
    gRfidReader->setLastSeenUid(lastUid);
    gModbusServer->setUid(lastUid);
    Serial.printf("[BOOT] Restored last UID: 0x%08lX\n", static_cast<unsigned long>(lastUid));
  }
}

void loop() {
  unsigned long currentMs = millis();

  // Servisleri çalıştır
  if (gModbusServer) gModbusServer->loop();
  if (gWebServerManager) gWebServerManager->loop();
  if (gWifiManager) gWifiManager->maintainConnection();

  // RFID & NFC Okuma Döngüsü
  if (currentMs - lastRfidCheckMs >= Config::RFID_TASK_DELAY_MS) {
    lastRfidCheckMs = currentMs;
    
    if (gRfidReader && gWifiManager) {
      uint32_t uid = 0;
      String configData = "";
      
      // AP modunda mıyız? 
      // isApMode() true ise NFC (CONFIG_TAG) ara, false ise RFID (NEW_UID) ara.
      bool isAp = gWifiManager->isApMode(); 
      
      CardResult result = gRfidReader->scanForCard(uid, configData, isAp);

      switch (result) {
        case CardResult::NEW_UID:
          gNvsManager->saveLastUid(uid); 
          gModbusServer->setUid(uid);          // Anlık
          gModbusServer->setLastUid(uid);      // Son okunan (Kalıcı)
          lastUidReadTime = currentMs; 
          Serial.printf("[SYSTEM] UID: 0x%08lX\n", (unsigned long)uid);
          break;

        case CardResult::CONFIG_TAG:
          Serial.println("[SYSTEM] NFC Configuration Tag Detected!");
          if (gWifiManager) {
            gWifiManager->updateCredentialsFromJson(configData);
          }
          break;

        case CardResult::NONE:
        default:
          break;
      }
    }
  }

  // 5 Saniyelik Otomatik Temizleme (Register ve Web Panel için)
  if (lastUidReadTime > 0 && (currentMs - lastUidReadTime >= 5000)) {
    gModbusServer->setUid(0);         
    gRfidReader->setLastSeenUid(0);   
    lastUidReadTime = 0;              
    Serial.println("[SYSTEM] 5 seconds timeout. Register cleared.");
  }
}