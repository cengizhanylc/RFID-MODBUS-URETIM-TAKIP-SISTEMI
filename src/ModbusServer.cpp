#include "ModbusServer.h"
#include "Config.h"

ModbusServer::ModbusServer() : started_(false), uid_(0) {}

bool ModbusServer::begin() {
  if (started_) return true;

  modbus_.server(Config::MODBUS_TCP_PORT);
  // Anlık Kart
  modbus_.addHreg(Config::MODBUS_HREG_UID_HIGH, 0);
  modbus_.addHreg(Config::MODBUS_HREG_UID_LOW, 0);
  // Son Okunan Kart
  modbus_.addHreg(Config::MODBUS_HREG_LAST_UID_HIGH, 0);
  modbus_.addHreg(Config::MODBUS_HREG_LAST_UID_LOW, 0);

  started_ = true;
  return true;
}

void ModbusServer::loop() {
  if (!started_) return;
  modbus_.task();
}

void ModbusServer::setUid(uint32_t uid) {
  uid_ = uid;
  if (!started_) return;
  uint16_t high = (uid >> 16) & 0xFFFF;
  uint16_t low  = uid & 0xFFFF;
  modbus_.Hreg(Config::MODBUS_HREG_UID_HIGH, high);
  modbus_.Hreg(Config::MODBUS_HREG_UID_LOW, low);
}

// Son okunan kart için (Kart çekilse bile değişmez)
void ModbusServer::setLastUid(uint32_t uid) {
  if (!started_) return;
  uint16_t high = (uid >> 16) & 0xFFFF;
  uint16_t low  = uid & 0xFFFF;
  modbus_.Hreg(Config::MODBUS_HREG_LAST_UID_HIGH, high);
  modbus_.Hreg(Config::MODBUS_HREG_LAST_UID_LOW, low);
}

uint32_t ModbusServer::getUid() const { return uid_; }