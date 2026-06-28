#ifndef MODBUS_SERVER_H
#define MODBUS_SERVER_H

#include <Arduino.h>
#include <ModbusIP_ESP8266.h>

class ModbusServer {
public:
  ModbusServer();
  bool begin();
  void loop();
  bool isStarted() const;
  void setUid(uint32_t uid);
  void setLastUid(uint32_t uid);
  uint32_t getUid() const;

private:
  ModbusIP modbus_;
  bool started_;
  uint32_t uid_;
};

#endif // MODBUS_SERVER_H
