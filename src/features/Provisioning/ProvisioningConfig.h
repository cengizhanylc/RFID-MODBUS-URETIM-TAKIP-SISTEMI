#ifndef PROVISIONING_CONFIG_H
#define PROVISIONING_CONFIG_H

#include <Arduino.h>

struct ProvisioningConfig {
  String ssid;
  String password;
  String ipAddress;
  String gateway;
  String subnet;

  bool hasSsid() const;
};

class ProvisioningConfigParser {
public:
  static bool parse(const String &payload, ProvisioningConfig &config);

private:
  static bool parseJson_(const String &payload, ProvisioningConfig &config);
  static bool parseKeyValueText_(const String &payload, ProvisioningConfig &config);
  static void applyField_(const String &key, const String &value, ProvisioningConfig &config);
  static String normalizedKey_(String key);
};

#endif // PROVISIONING_CONFIG_H
