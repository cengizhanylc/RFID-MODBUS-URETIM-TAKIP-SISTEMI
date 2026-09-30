#include "BootButtonReset.h"

#include "Config.h"

void BootButtonReset::begin() {
  pinMode(Config::BOOT_BUTTON_PIN, INPUT_PULLUP);

  rawPressed_ = digitalRead(Config::BOOT_BUTTON_PIN) == LOW;
  stablePressed_ = rawPressed_;
  armed_ = !stablePressed_;
  factoryResetPending_ = false;
  factoryResetRequest_ = false;
  lastRawChangeMs_ = millis();
}

void BootButtonReset::loop(uint32_t currentMs) {
  const bool pressed = digitalRead(Config::BOOT_BUTTON_PIN) == LOW;
  if (pressed != rawPressed_) {
    rawPressed_ = pressed;
    lastRawChangeMs_ = currentMs;
  }

  if (rawPressed_ == stablePressed_ ||
      (currentMs - lastRawChangeMs_) < Config::BOOT_BUTTON_DEBOUNCE_MS) {
    return;
  }

  stablePressed_ = rawPressed_;
  if (!stablePressed_) {
    armed_ = true;
    if (!factoryResetPending_) return;

    factoryResetPending_ = false;
    factoryResetRequest_ = true;
    Serial.println("[BOOT] BOOT button released. Factory reset requested.");
    return;
  }

  if (!armed_) return;

  factoryResetPending_ = true;
  Serial.println("[BOOT] BOOT button pressed. Release to restore factory settings...");
}

bool BootButtonReset::consumeFactoryResetRequest() {
  if (!factoryResetRequest_) return false;

  factoryResetRequest_ = false;
  return true;
}
