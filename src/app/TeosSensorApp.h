#ifndef TEOS_SENSOR_APP_H
#define TEOS_SENSOR_APP_H

#include <Arduino.h>

#include "../features/BootButtonReset/BootButtonReset.h"
#include "../ModbusServer.h"
#include "../NvsManager.h"
#include "../RfidReader.h"
#include "../WebServerManager.h"
#include "../WifiManager.h"

class TeosSensorApp {
public:
  TeosSensorApp();

  void setup();
  void loop();

private:
  bool initEspNvsFlash_();
  void initializeHardware_();
  void initializeServices_();
  void handleBootButton_(uint32_t currentMs);
  void restoreLastUid_();
  void updateIndicators_(uint32_t currentMs);
  void signalCardRead_(uint32_t currentMs);
  void pollRfid_(uint32_t currentMs);
  void clearExpiredUid_(uint32_t currentMs);
  void startModbusIfSta_();

  NvsManager nvsManager_;
  WifiManager wifiManager_;
  WebServerManager webServerManager_;
  RfidReader rfidReader_;
  ModbusServer modbusServer_;
  BootButtonReset bootButtonReset_;
  uint32_t lastRfidCheckMs_;
  uint32_t lastUidReadTime_;
  uint32_t lastApLedToggleMs_;
  uint32_t lastStaLedToggleMs_;
  uint32_t buzzerStartMs_;
  uint32_t readLedStartMs_;
  bool buzzerActive_;
  bool readLedActive_;
};

#endif // TEOS_SENSOR_APP_H
