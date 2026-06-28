#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "NvsManager.h"
#include "features/Provisioning/ProvisioningConfig.h"

class WifiManager {
public:
  enum class Mode {
    None,
    AP,
    STA
  };

  explicit WifiManager(NvsManager &nvsManager);

  bool begin();
  void maintainConnection();

  Mode getMode() const;
  bool isApMode() const;
  bool isStaMode() const;

  bool updateCredentialsFromProvisioningPayload(const String& payload);
  bool updateCredentialsFromJson(const String& jsonString);

private:
  bool startSta_();
  bool startAp_();
  bool applyStaNetworkConfig_();
  bool switchToStaAfterProvisioning_();
  bool saveProvisioningConfig_(const ProvisioningConfig &config);
  bool isValidIpv4_(const String &ipText) const;

  NvsManager &nvsManager_;
  Mode mode_;
  uint32_t lastReconnectAttemptMs_;
  String staSsid_;
  String staPassword_;
  String staStaticIp_;
  String staGateway_;
  String staSubnet_;
};

#endif // WIFI_MANAGER_H
