#include "WifiManager.h"
#include "Config.h"

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

bool WifiManager::updateCredentialsFromProvisioningPayload(const String& payload) {
  Serial.printf("[WiFi] NFC payload received (%d bytes): %s\n", payload.length(), payload.c_str());

  ProvisioningConfig config;
  if (!ProvisioningConfigParser::parse(payload, config)) {
    Serial.println("[WiFi] NFC provisioning payload could not be parsed.");
    return false;
  }

  Serial.printf("[WiFi] Parsed -> SSID: '%s', IP: '%s', GW: '%s', Subnet: '%s'\n",
    config.ssid.c_str(), config.ipAddress.c_str(), config.gateway.c_str(), config.subnet.c_str());

  if (!saveProvisioningConfig_(config)) {
    Serial.println("[WiFi] NFC provisioning config could not be saved.");
    return false;
  }

  Serial.println("[WiFi] NFC config saved to NVS. Switching to STA mode...");
  return switchToStaAfterProvisioning_();
}

bool WifiManager::updateCredentialsFromJson(const String& jsonString) {
  return updateCredentialsFromProvisioningPayload(jsonString);
}

bool WifiManager::startSta_() {
  if (!nvsManager_.getWifiCredentials(staSsid_, staPassword_)) {
    Serial.println("[WiFi] No stored credentials found.");
    return false;
  }

  Serial.printf("[WiFi] STA credentials loaded -> SSID: '%s', Pass length: %d\n", 
    staSsid_.c_str(), staPassword_.length());

  if (!nvsManager_.getStaNetworkConfig(staStaticIp_, staGateway_, staSubnet_)) {
    staStaticIp_ = Config::WIFI_STA_DEFAULT_STATIC_IP;
    staGateway_ = Config::WIFI_STA_DEFAULT_GATEWAY;
    staSubnet_ = Config::WIFI_STA_DEFAULT_SUBNET;
    Serial.println("[WiFi] No stored network config, using defaults.");
  }

  if (staStaticIp_.length() == 0) staStaticIp_ = Config::WIFI_STA_DEFAULT_STATIC_IP;
  if (staGateway_.length() == 0) staGateway_ = Config::WIFI_STA_DEFAULT_GATEWAY;
  if (staSubnet_.length() == 0) staSubnet_ = Config::WIFI_STA_DEFAULT_SUBNET;

  Serial.printf("[WiFi] Network config -> IP: %s, GW: %s, Subnet: %s\n",
    staStaticIp_.c_str(), staGateway_.c_str(), staSubnet_.c_str());

  Serial.printf("[WiFi] Connecting to SSID: %s\n", staSsid_.c_str());
  
  WiFi.mode(WIFI_STA);
  digitalWrite(Config::LED_AP_PIN, HIGH); // STA'ya geçildi, AP ledi sabit yanar

  WiFi.disconnect(true); // Önceki bağlantı durumunu temizle
  delay(100);

  if (!applyStaNetworkConfig_()) {
    Serial.println("[WiFi] WARNING: Static IP config failed, continuing with DHCP...");
  }

  WiFi.begin(staSsid_.c_str(), staPassword_.c_str());

  const uint32_t startMs = millis();
  const uint32_t timeoutMs = 20000; // 10sn yetmeyebilir, 20sn'ye çıkarıldı

  while (WiFi.status() != WL_CONNECTED) {
    if ((millis() - startMs) >= timeoutMs) {
      Serial.printf("[WiFi] STA connection timed out after %lu ms. WiFi.status()=%d\n",
        timeoutMs, WiFi.status());
      WiFi.disconnect();
      digitalWrite(Config::LED_STA_PIN, LOW);
      return false;
    }
    digitalWrite(Config::LED_STA_PIN, !digitalRead(Config::LED_STA_PIN));
    vTaskDelay(pdMS_TO_TICKS(500));
    Serial.print(".");
  }
  Serial.println();

  digitalWrite(Config::LED_STA_PIN, HIGH); // Bağlandı

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

bool WifiManager::switchToStaAfterProvisioning_() {
  // Config NVS'e başarıyla kaydedildi.
  // WiFi stack'i bozmamak (netstack cb reg failed hatası) için
  // hot-switch yerine cihazı doğrudan yeniden başlatıyoruz.
  Serial.println("[WiFi] Provisioning complete. Rebooting to apply NFC config...");
  delay(1000);
  ESP.restart();
  return false; 
}

bool WifiManager::saveProvisioningConfig_(const ProvisioningConfig &config) {
  if (config.ssid.length() == 0) {
    Serial.println("[WiFi] ERROR: SSID cannot be empty in NFC data.");
    return false;
  }

  String password = config.password;
  if (password.length() == 0) {
    String storedSsid, storedPassword;
    if (nvsManager_.getWifiCredentials(storedSsid, storedPassword) && storedSsid == config.ssid) {
      password = storedPassword;
    }
  }

  const String staticIp = config.ipAddress.length() > 0 ? config.ipAddress : Config::WIFI_STA_DEFAULT_STATIC_IP;
  const String gateway = config.gateway.length() > 0 ? config.gateway : Config::WIFI_STA_DEFAULT_GATEWAY;
  const String subnet = config.subnet.length() > 0 ? config.subnet : Config::WIFI_STA_DEFAULT_SUBNET;

  if (!isValidIpv4_(staticIp) || !isValidIpv4_(gateway) || !isValidIpv4_(subnet)) {
    Serial.println("[WiFi] ERROR: NFC network settings contain invalid IPv4 value.");
    return false;
  }

  const bool credentialsSaved = nvsManager_.saveWifiCredentials(config.ssid, password);
  const bool networkSaved = nvsManager_.saveStaNetworkConfig(staticIp, gateway, subnet);

  if (networkSaved) Serial.println("[WiFi] Static IP settings saved via NFC.");
  return credentialsSaved && networkSaved;
}

bool WifiManager::isValidIpv4_(const String &ipText) const {
  IPAddress ipAddress;
  return ipAddress.fromString(ipText);
}
