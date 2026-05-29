#ifndef NVS_MANAGER_H
#define NVS_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

class NvsManager {
public:
  NvsManager();
  ~NvsManager();

  bool begin();
  bool hasWifiCredentials();
  bool saveWifiCredentials(const String &ssid, const String &password);
  bool getWifiCredentials(String &ssid, String &password);

  bool saveStaStaticIp(const String &ipAddress);
  bool getStaStaticIp(String &ipAddress);
  
  bool saveStaNetworkConfig(const String &ipAddress, const String &gateway, const String &subnet);
  bool getStaNetworkConfig(String &ipAddress, String &gateway, String &subnet);
  bool clearStaNetworkConfig();

  bool saveLastUid(uint32_t uid);
  bool getLastUid(uint32_t &uid);

  bool factoryReset();

private:
  Preferences preferences_;
  bool initialized_;
};

#endif // NVS_MANAGER_H