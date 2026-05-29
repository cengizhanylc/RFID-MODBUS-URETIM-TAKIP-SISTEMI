#include "NvsManager.h"
#include "Config.h"

NvsManager::NvsManager() : initialized_(false) {}

NvsManager::~NvsManager() {
  if (initialized_) {
    preferences_.end();
    initialized_ = false;
  }
}

bool NvsManager::begin() {
  if (initialized_) return true;
  initialized_ = preferences_.begin(Config::NVS_NAMESPACE, false);
  return initialized_;
}

bool NvsManager::hasWifiCredentials() {
  if (!initialized_) return false;
  const String ssid = preferences_.getString(Config::NVS_KEY_WIFI_SSID, "");
  return ssid.length() > 0;
}

bool NvsManager::saveWifiCredentials(const String &ssid, const String &password) {
  if (!initialized_ || ssid.length() == 0) return false;
  const bool ssidOk = preferences_.putString(Config::NVS_KEY_WIFI_SSID, ssid) > 0;
  bool passOk = false;
  if (password.length() == 0) {
    (void)preferences_.remove(Config::NVS_KEY_WIFI_PASS);
    passOk = true;
  } else {
    passOk = preferences_.putString(Config::NVS_KEY_WIFI_PASS, password) > 0;
  }
  return ssidOk && passOk;
}

bool NvsManager::getWifiCredentials(String &ssid, String &password) {
  if (!initialized_) return false;
  ssid = preferences_.getString(Config::NVS_KEY_WIFI_SSID, "");
  password = preferences_.getString(Config::NVS_KEY_WIFI_PASS, "");
  return ssid.length() > 0;
}

bool NvsManager::saveStaStaticIp(const String &ipAddress) {
  if (!initialized_ || ipAddress.length() == 0) return false;
  return preferences_.putString(Config::NVS_KEY_STA_STATIC_IP, ipAddress) > 0;
}

bool NvsManager::getStaStaticIp(String &ipAddress) {
  if (!initialized_) return false;
  ipAddress = preferences_.getString(Config::NVS_KEY_STA_STATIC_IP, "");
  return preferences_.isKey(Config::NVS_KEY_STA_STATIC_IP) && ipAddress.length() > 0;
}

bool NvsManager::saveStaNetworkConfig(const String &ipAddress, const String &gateway, const String &subnet) {
  if (!initialized_ || ipAddress.length() == 0 || gateway.length() == 0 || subnet.length() == 0) return false;
  const bool ipOk = preferences_.putString(Config::NVS_KEY_STA_STATIC_IP, ipAddress) > 0;
  const bool gwOk = preferences_.putString(Config::NVS_KEY_STA_GATEWAY, gateway) > 0;
  const bool subnetOk = preferences_.putString(Config::NVS_KEY_STA_SUBNET, subnet) > 0;
  return ipOk && gwOk && subnetOk;
}

bool NvsManager::getStaNetworkConfig(String &ipAddress, String &gateway, String &subnet) {
  if (!initialized_) return false;
  ipAddress = preferences_.getString(Config::NVS_KEY_STA_STATIC_IP, "");
  gateway = preferences_.getString(Config::NVS_KEY_STA_GATEWAY, "");
  subnet = preferences_.getString(Config::NVS_KEY_STA_SUBNET, "");
  const bool hasIp = preferences_.isKey(Config::NVS_KEY_STA_STATIC_IP) && ipAddress.length() > 0;
  const bool hasGateway = preferences_.isKey(Config::NVS_KEY_STA_GATEWAY) && gateway.length() > 0;
  const bool hasSubnet = preferences_.isKey(Config::NVS_KEY_STA_SUBNET) && subnet.length() > 0;
  return hasIp && hasGateway && hasSubnet;
}

bool NvsManager::clearStaNetworkConfig() {
  if (!initialized_) return false;
  preferences_.remove(Config::NVS_KEY_STA_STATIC_IP);
  preferences_.remove(Config::NVS_KEY_STA_GATEWAY);
  preferences_.remove(Config::NVS_KEY_STA_SUBNET);
  return true; 
}

bool NvsManager::saveLastUid(uint32_t uid) {
  if (!initialized_) return false;
  return preferences_.putULong(Config::NVS_KEY_LAST_UID, uid) > 0;
}

bool NvsManager::getLastUid(uint32_t &uid) {
  if (!initialized_) return false;
  uid = preferences_.getULong(Config::NVS_KEY_LAST_UID, 0);
  return preferences_.isKey(Config::NVS_KEY_LAST_UID);
}

bool NvsManager::factoryReset() {
  if (!initialized_) return false;
  return preferences_.clear();
}