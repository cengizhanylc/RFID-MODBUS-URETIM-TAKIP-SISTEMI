#include "ProvisioningConfig.h"

#include <ArduinoJson.h>

bool ProvisioningConfig::hasSsid() const {
  return ssid.length() > 0;
}

bool ProvisioningConfigParser::parse(const String &payload, ProvisioningConfig &config) {
  String normalizedPayload = payload;
  normalizedPayload.trim();
  if (normalizedPayload.length() == 0) return false;

  config = ProvisioningConfig();
  if (parseJson_(normalizedPayload, config) && config.hasSsid()) return true;

  config = ProvisioningConfig();
  if (parseKeyValueText_(normalizedPayload, config) && config.hasSsid()) return true;

  return false;
}

bool ProvisioningConfigParser::parseJson_(const String &payload, ProvisioningConfig &config) {
  if (!payload.startsWith("{")) return false;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) return false;

  config.ssid = doc["ssid"] | "";
  if (config.ssid.length() == 0) config.ssid = doc["s"] | "";

  config.password = doc["password"] | "";
  if (config.password.length() == 0) config.password = doc["pass"] | "";
  if (config.password.length() == 0) config.password = doc["p"] | "";

  config.ipAddress = doc["ip"] | "";
  if (config.ipAddress.length() == 0) config.ipAddress = doc["i"] | "";

  config.gateway = doc["gateway"] | "";
  if (config.gateway.length() == 0) config.gateway = doc["gw"] | "";
  if (config.gateway.length() == 0) config.gateway = doc["g"] | "";

  config.subnet = doc["subnet"] | "";
  if (config.subnet.length() == 0) config.subnet = doc["mask"] | "";
  if (config.subnet.length() == 0) config.subnet = doc["netmask"] | "";
  if (config.subnet.length() == 0) config.subnet = doc["n"] | "";

  config.ssid.trim();
  config.password.trim();
  config.ipAddress.trim();
  config.gateway.trim();
  config.subnet.trim();
  return true;
}

bool ProvisioningConfigParser::parseKeyValueText_(const String &payload, ProvisioningConfig &config) {
  String normalizedPayload = payload;
  normalizedPayload.replace("\r", "\n");
  normalizedPayload.replace(";", "\n");
  normalizedPayload.replace(",", "\n");

  int lineStart = 0;
  while (lineStart < normalizedPayload.length()) {
    int lineEnd = normalizedPayload.indexOf('\n', lineStart);
    if (lineEnd < 0) lineEnd = normalizedPayload.length();

    String line = normalizedPayload.substring(lineStart, lineEnd);
    line.trim();

    if (line.length() > 0) {
      int separatorIndex = line.indexOf('=');
      if (separatorIndex < 0) separatorIndex = line.indexOf(':');

      if (separatorIndex > 0) {
        String key = line.substring(0, separatorIndex);
        String value = line.substring(separatorIndex + 1);
        key.trim();
        value.trim();
        applyField_(key, value, config);
      }
    }

    lineStart = lineEnd + 1;
  }

  return true;
}

void ProvisioningConfigParser::applyField_(const String &key, const String &value, ProvisioningConfig &config) {
  const String normalizedKey = normalizedKey_(key);

  if (normalizedKey == "ssid" || normalizedKey == "wifissid" || normalizedKey == "s") {
    config.ssid = value;
  } else if (normalizedKey == "password" || normalizedKey == "pass" || normalizedKey == "wifipass" ||
             normalizedKey == "sifre" || normalizedKey == "p") {
    config.password = value;
  } else if (normalizedKey == "ip" || normalizedKey == "i" || normalizedKey == "ipaddress" ||
             normalizedKey == "ipadresi") {
    config.ipAddress = value;
  } else if (normalizedKey == "gateway" || normalizedKey == "gatewayip" || normalizedKey == "gw" ||
             normalizedKey == "g") {
    config.gateway = value;
  } else if (normalizedKey == "subnet" || normalizedKey == "subnetmask" || normalizedKey == "mask" ||
             normalizedKey == "netmask" || normalizedKey == "n") {
    config.subnet = value;
  }
}

String ProvisioningConfigParser::normalizedKey_(String key) {
  key.trim();
  key.toLowerCase();
  key.replace(" ", "");
  key.replace("_", "");
  key.replace("-", "");
  return key;
}
