#include "TeosSensorApp.h"

#include <nvs_flash.h>

#include "Config.h"

TeosSensorApp::TeosSensorApp()
    : nvsManager_(),
      wifiManager_(nvsManager_),
      webServerManager_(nvsManager_),
      rfidReader_(),
      modbusServer_(),
      lastRfidCheckMs_(0),
      lastUidReadTime_(0) {}

void TeosSensorApp::setup() {
  delay(2000);
  Serial.begin(115200);
  Serial.println("\n[BOOT] TeosSensor starting...");

  if (!initEspNvsFlash_()) Serial.println("[BOOT] Failed to initialize NVS.");

  initializeServices_();
  restoreLastUid_();
}

void TeosSensorApp::loop() {
  const uint32_t currentMs = millis();

  modbusServer_.loop();
  webServerManager_.loop();
  wifiManager_.maintainConnection();

  pollRfid_(currentMs);
  clearExpiredUid_(currentMs);
}

bool TeosSensorApp::initEspNvsFlash_() {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_OK) return true;
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    if (nvs_flash_erase() != ESP_OK) return false;
    err = nvs_flash_init();
    return err == ESP_OK;
  }
  return false;
}

void TeosSensorApp::initializeServices_() {
  if (!nvsManager_.begin()) Serial.println("[BOOT] NVS init failed.");
  if (!rfidReader_.begin()) Serial.println("[BOOT] RFID init failed.");
  if (!wifiManager_.begin()) Serial.println("[BOOT] Wi-Fi init failed.");

  webServerManager_.begin();

  startModbusIfSta_();
}

void TeosSensorApp::restoreLastUid_() {
  uint32_t lastUid = 0;
  if (!nvsManager_.getLastUid(lastUid)) return;

  rfidReader_.setLastSeenUid(lastUid);
  modbusServer_.setUid(lastUid);
  Serial.printf("[BOOT] Restored last UID: 0x%08lX\n", static_cast<unsigned long>(lastUid));
}

void TeosSensorApp::pollRfid_(uint32_t currentMs) {
  if (currentMs - lastRfidCheckMs_ < Config::RFID_TASK_DELAY_MS) return;
  lastRfidCheckMs_ = currentMs;

  uint32_t uid = 0;
  String configData = "";
  const bool isAp = wifiManager_.isApMode();
  const CardResult result = rfidReader_.scanForCard(uid, configData, isAp);

  switch (result) {
    case CardResult::NEW_UID:
      nvsManager_.saveLastUid(uid);
      modbusServer_.setUid(uid);
      modbusServer_.setLastUid(uid);
      lastUidReadTime_ = currentMs;
      Serial.printf("[SYSTEM] UID: 0x%08lX\n", static_cast<unsigned long>(uid));
      break;

    case CardResult::CONFIG_TAG:
      Serial.println("[SYSTEM] NFC Configuration Tag Detected!");
      if (wifiManager_.updateCredentialsFromProvisioningPayload(configData)) startModbusIfSta_();
      break;

    case CardResult::NONE:
    default:
      break;
  }
}

void TeosSensorApp::clearExpiredUid_(uint32_t currentMs) {
  if (lastUidReadTime_ == 0) return;
  if (currentMs - lastUidReadTime_ < Config::RFID_UID_CLEAR_TIMEOUT_MS) return;

  modbusServer_.setUid(0);
  rfidReader_.setLastSeenUid(0);
  lastUidReadTime_ = 0;
  Serial.println("[SYSTEM] UID timeout. Register cleared.");
}

void TeosSensorApp::startModbusIfSta_() {
  if (!wifiManager_.isStaMode() || modbusServer_.isStarted()) return;
  if (!modbusServer_.begin()) Serial.println("[BOOT] Modbus init failed.");
}
