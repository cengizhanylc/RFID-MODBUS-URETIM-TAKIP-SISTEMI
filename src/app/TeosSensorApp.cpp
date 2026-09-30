#include "TeosSensorApp.h"

#include <WiFi.h>
#include <nvs_flash.h>

#include "Config.h"

TeosSensorApp::TeosSensorApp()
    : nvsManager_(),
      wifiManager_(nvsManager_),
      webServerManager_(nvsManager_),
      rfidReader_(),
      modbusServer_(),
      bootButtonReset_(),
      lastRfidCheckMs_(0),
      lastUidReadTime_(0),
      lastApLedToggleMs_(0),
      lastStaLedToggleMs_(0),
      buzzerStartMs_(0),
      readLedStartMs_(0),
      buzzerActive_(false),
      readLedActive_(false) {}

void TeosSensorApp::setup() {
  initializeHardware_();
  delay(2000);
  Serial.begin(115200);
  Serial.println("\n[BOOT] TeosSensor starting...");

  if (!initEspNvsFlash_()) Serial.println("[BOOT] Failed to initialize NVS.");

  initializeServices_();
  restoreLastUid_();
}

void TeosSensorApp::loop() {
  const uint32_t currentMs = millis();

  handleBootButton_(currentMs);
  modbusServer_.loop();
  webServerManager_.loop();
  wifiManager_.maintainConnection();

  updateIndicators_(currentMs);
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

void TeosSensorApp::initializeHardware_() {
  pinMode(Config::LED_AP_PIN, OUTPUT);
  pinMode(Config::LED_STA_PIN, OUTPUT);
  pinMode(Config::BUZZER_PIN, OUTPUT);
  pinMode(Config::RFID_IS_READ_PIN, OUTPUT);

  digitalWrite(Config::LED_AP_PIN, LOW);
  digitalWrite(Config::LED_STA_PIN, LOW);
  digitalWrite(Config::BUZZER_PIN, LOW);
  digitalWrite(Config::RFID_IS_READ_PIN, LOW);

  bootButtonReset_.begin();
}

void TeosSensorApp::handleBootButton_(uint32_t currentMs) {
  bootButtonReset_.loop(currentMs);
  if (!bootButtonReset_.consumeFactoryResetRequest()) return;

  Serial.println("[BOOT] Clearing stored settings...");
  if (!nvsManager_.factoryReset()) {
    Serial.println("[BOOT] Factory reset failed. Stored settings were not cleared.");
    return;
  }

  Serial.println("[BOOT] Factory reset complete. Restarting MCU...");
  delay(100);
  ESP.restart();
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

void TeosSensorApp::updateIndicators_(uint32_t currentMs) {
  if (wifiManager_.isApMode()) {
    if (currentMs - lastApLedToggleMs_ >= 1000) {
      lastApLedToggleMs_ = currentMs;
      digitalWrite(Config::LED_AP_PIN, !digitalRead(Config::LED_AP_PIN));
    }
    digitalWrite(Config::LED_STA_PIN, LOW);
  } else if (wifiManager_.isStaMode()) {
    digitalWrite(Config::LED_AP_PIN, HIGH);

    if (WiFi.status() == WL_CONNECTED) {
      digitalWrite(Config::LED_STA_PIN, HIGH);
    } else if (currentMs - lastStaLedToggleMs_ >= 500) {
      lastStaLedToggleMs_ = currentMs;
      digitalWrite(Config::LED_STA_PIN, !digitalRead(Config::LED_STA_PIN));
    }
  }

  if (buzzerActive_ && (currentMs - buzzerStartMs_ >= 1000)) {
    noTone(Config::BUZZER_PIN);
    buzzerActive_ = false;
  }

  if (readLedActive_ && (currentMs - readLedStartMs_ >= 2000)) {
    digitalWrite(Config::RFID_IS_READ_PIN, LOW);
    readLedActive_ = false;
  }
}

void TeosSensorApp::signalCardRead_(uint32_t currentMs) {
  tone(Config::BUZZER_PIN, 2000);
  buzzerActive_ = true;
  buzzerStartMs_ = currentMs;

  digitalWrite(Config::RFID_IS_READ_PIN, HIGH);
  readLedActive_ = true;
  readLedStartMs_ = currentMs;
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
      signalCardRead_(currentMs);
      Serial.printf("[SYSTEM] UID: 0x%08lX\n", static_cast<unsigned long>(uid));
      break;

    case CardResult::CONFIG_TAG:
      Serial.println("[SYSTEM] NFC Configuration Tag Detected!");
      signalCardRead_(currentMs);
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
