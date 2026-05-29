#include "WifiManager.h"
#include "Config.h"
#include <ArduinoJson.h> 

WifiManager::WifiManager(NvsManager &nvsManager)
    : nvsManager_(nvsManager), mode_(Mode::None), lastReconnectAttemptMs_(0) {}

bool WifiManager::begin() {
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);

  if (startSta_()) {
    mode_ = Mode::STA;
    return true;
  }
  return startAp_();
}

void WifiManager::maintainConnection() {
  if (mode_ != Mode::STA) return;
  if (WiFi.status() == WL_CONNECTED) return;

  const uint32_t now = millis();
  if ((now - lastReconnectAttemptMs_) < Config::WIFI_RECONNECT_INTERVAL_MS) return;

  lastReconnectAttemptMs_ = now;
  Serial.println("[WiFi] STA disconnected. Attempting reconnect...");

  if (staSsid_.length() > 0) {
    WiFi.disconnect();
    applyStaNetworkConfig_();
    WiFi.begin(staSsid_.c_str(), staPassword_.c_str());
  } else {
    WiFi.reconnect();
  }
}

WifiManager::Mode WifiManager::getMode() const { return mode_; }
bool WifiManager::isApMode() const { return mode_ == Mode::AP; }
bool WifiManager::isStaMode() const { return mode_ == Mode::STA; }

bool WifiManager::updateCredentialsFromJson(const String& jsonString) {
  JsonDocument doc; 
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) {
    Serial.printf("[WiFi] NFC JSON Parse failed: %s\n", error.c_str());
    return false;
  }

  Serial.println("[WiFi] NFC config received. Processing...");

  String ssid = doc["s"] | "";
  String pass = doc["p"] | "";
  String ip   = doc["i"] | "";
  String gw   = doc["g"] | "";
  String sub  = doc["n"] | "";

  if (ssid.length() > 0) {
    nvsManager_.saveWifiCredentials(ssid, pass);

    if (ip.length() > 0 && gw.length() > 0) {
      nvsManager_.saveStaNetworkConfig(ip, gw, sub);
      Serial.println("[WiFi] Static IP settings saved via NFC.");
    } else {
      nvsManager_.saveStaNetworkConfig(Config::WIFI_STA_DEFAULT_STATIC_IP, Config::WIFI_STA_DEFAULT_GATEWAY, Config::WIFI_STA_DEFAULT_SUBNET);
      Serial.println("[WiFi] Static IP settings reset to default via NFC.");
    }

    Serial.println("[WiFi] NFC config saved to NVS. Restarting ESP...");
    delay(1000); 
    ESP.restart(); 
    return true;
  }

  Serial.println("[WiFi] ERROR: SSID cannot be empty in NFC data.");
  return false;
}

bool WifiManager::startSta_() {
  if (!nvsManager_.getWifiCredentials(staSsid_, staPassword_)) {
    Serial.println("[WiFi] No stored credentials found.");
    return false;
  }

  if (!nvsManager_.getStaNetworkConfig(staStaticIp_, staGateway_, staSubnet_)) {
    staStaticIp_ = Config::WIFI_STA_DEFAULT_STATIC_IP;
    staGateway_ = Config::WIFI_STA_DEFAULT_GATEWAY;
    staSubnet_ = Config::WIFI_STA_DEFAULT_SUBNET;
  }

  if (staStaticIp_.length() == 0) staStaticIp_ = Config::WIFI_STA_DEFAULT_STATIC_IP;
  if (staGateway_.length() == 0) staGateway_ = Config::WIFI_STA_DEFAULT_GATEWAY;
  if (staSubnet_.length() == 0) staSubnet_ = Config::WIFI_STA_DEFAULT_SUBNET;

  Serial.printf("[WiFi] Connecting to SSID: %s\n", staSsid_.c_str());
  WiFi.mode(WIFI_STA);
  applyStaNetworkConfig_();
  WiFi.begin(staSsid_.c_str(), staPassword_.c_str());

  const uint32_t startMs = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if ((millis() - startMs) >= Config::WIFI_STA_CONNECT_TIMEOUT_MS) {
      Serial.println("[WiFi] STA connection timed out.");
      WiFi.disconnect();
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(250));
  }

  Serial.print("[WiFi] STA connected. IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

bool WifiManager::startAp_() {
  Serial.println("[WiFi] Starting provisioning AP mode...");
  WiFi.mode(WIFI_AP);

  const bool started = WiFi.softAP(Config::AP_SSID, Config::AP_PASSWORD);
  if (!started) {
    mode_ = Mode::None;
    Serial.println("[WiFi] AP startup failed.");
    return false;
  }

  mode_ = Mode::AP;
  Serial.print("[WiFi] AP ready. IP: ");
  Serial.println(WiFi.softAPIP());
  return true;
}

bool WifiManager::applyStaNetworkConfig_() {
  IPAddress localIp, gateway, subnet;

  if (!localIp.fromString(staStaticIp_)) {
    staStaticIp_ = Config::WIFI_STA_DEFAULT_STATIC_IP;
    if (!localIp.fromString(staStaticIp_)) return false;
  }
  if (!gateway.fromString(staGateway_)) {
    staGateway_ = Config::WIFI_STA_DEFAULT_GATEWAY;
    if (!gateway.fromString(staGateway_)) return false;
  }
  if (!subnet.fromString(staSubnet_)) {
    staSubnet_ = Config::WIFI_STA_DEFAULT_SUBNET;
    if (!subnet.fromString(staSubnet_)) return false;
  }

  if (!WiFi.config(localIp, gateway, subnet)) {
    Serial.println("[WiFi] Failed to apply static IP.");
    return false;
  }

  return true;
}