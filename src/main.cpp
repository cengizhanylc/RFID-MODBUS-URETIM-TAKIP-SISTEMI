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

unsigned long lastLed47ToggleMs = 0;
unsigned long lastLed48ToggleMs = 0;
unsigned long buzzerStartMs = 0;
bool buzzerActive = false;

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
  pinMode(Config::LED_AP_PIN, OUTPUT);
  pinMode(Config::LED_STA_PIN, OUTPUT);
  pinMode(Config::BUZZER_PIN, OUTPUT);
  digitalWrite(Config::LED_AP_PIN, LOW);
  digitalWrite(Config::LED_STA_PIN, LOW);
  digitalWrite(Config::BUZZER_PIN, LOW);

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

  // Gösterge (LED) Yönetimi
  if (gWifiManager) {
    if (gWifiManager->isApMode()) {
      // AP modunda LED 47 1 sn aralıkla yanıp söner
      if (currentMs - lastLed47ToggleMs >= 1000) {
        lastLed47ToggleMs = currentMs;
        digitalWrite(Config::LED_AP_PIN, !digitalRead(Config::LED_AP_PIN));
      }
      digitalWrite(Config::LED_STA_PIN, LOW);
    } else if (gWifiManager->isStaMode()) {
      // STA modunda LED 47 sabit yanar
      digitalWrite(Config::LED_AP_PIN, HIGH);
      
      // STA modunda bağlantı koptuğunda reconnect sırasında LED 48 yanıp söner
      if (WiFi.status() == WL_CONNECTED) {
        digitalWrite(Config::LED_STA_PIN, HIGH);
      } else {
        if (currentMs - lastLed48ToggleMs >= 500) {
          lastLed48ToggleMs = currentMs;
          digitalWrite(Config::LED_STA_PIN, !digitalRead(Config::LED_STA_PIN));
        }
      }
    }
  }

  // Buzzer Yönetimi (1 saniye sonra susar)
  if (buzzerActive && (currentMs - buzzerStartMs >= 1000)) {
    noTone(Config::BUZZER_PIN);
    buzzerActive = false;
  }

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
          
          tone(Config::BUZZER_PIN, 2000); 
          buzzerActive = true;
          buzzerStartMs = currentMs;

          Serial.printf("[SYSTEM] UID: 0x%08lX\n", (unsigned long)uid);
          break;

        case CardResult::CONFIG_TAG:
          Serial.println("[SYSTEM] NFC Configuration Tag Detected!");
          tone(Config::BUZZER_PIN, 2000); 
          buzzerActive = true;
          buzzerStartMs = currentMs;

          if (gWifiManager) {
            const bool switchedToSta = gWifiManager->updateCredentialsFromProvisioningPayload(configData);
            if (switchedToSta && gModbusServer && !gModbusServer->isStarted()) {
              if (!gModbusServer->begin()) Serial.println("[SYSTEM] Modbus init failed after NFC provisioning.");
            }
          }
          break;

        case CardResult::NONE:
        default:
          break;
      }
    }
  }

  // 5 Saniyelik Otomatik Temizleme (Register ve Web Panel için)
  if (lastUidReadTime > 0 && (currentMs - lastUidReadTime >= Config::RFID_UID_CLEAR_TIMEOUT_MS)) {
    gModbusServer->setUid(0);         
    gRfidReader->setLastSeenUid(0);   
    lastUidReadTime = 0;              
    Serial.println("[SYSTEM] 5 seconds timeout. Register cleared.");
  }
}
