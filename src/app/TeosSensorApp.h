#ifndef TEOS_SENSOR_APP_H
#define TEOS_SENSOR_APP_H

#include <Arduino.h>

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
  void initializeServices_();
  void restoreLastUid_();
  void pollRfid_(uint32_t currentMs);
  void clearExpiredUid_(uint32_t currentMs);
  void startModbusIfSta_();

  NvsManager nvsManager_;
  WifiManager wifiManager_;
  WebServerManager webServerManager_;
  RfidReader rfidReader_;
  ModbusServer modbusServer_;
  uint32_t lastRfidCheckMs_;
  uint32_t lastUidReadTime_;
};

#endif // TEOS_SENSOR_APP_H
