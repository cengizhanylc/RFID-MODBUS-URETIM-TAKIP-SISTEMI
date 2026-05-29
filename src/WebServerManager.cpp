#include "WebServerManager.h"
#include "Config.h"
#include <WiFi.h>

namespace {
bool isValidIpv4(const String &ipText) {
  IPAddress ip;
  return ip.fromString(ipText);
}
String escapeHtml(const String &raw) {
  String escaped = raw;
  escaped.replace("&", "&amp;");
  escaped.replace("<", "&lt;");
  escaped.replace(">", "&gt;");
  escaped.replace("\"", "&quot;");
  escaped.replace("'", "&#39;");
  return escaped;
}
String escapeJson(const String &raw) {
  String escaped = raw;
  escaped.replace("\\", "\\\\");
  escaped.replace("\"", "\\\"");
  escaped.replace("\n", "\\n");
  escaped.replace("\r", "\\r");
  escaped.replace("\t", "\\t");
  return escaped;
}
String formatUidHex(uint32_t uid) {
  char buffer[11];
  snprintf(buffer, sizeof(buffer), "0x%08lX", static_cast<unsigned long>(uid));
  return String(buffer);
}
String wifiModeLabel() {
  const wifi_mode_t mode = WiFi.getMode();
  if (mode == WIFI_MODE_AP) return "AP";
  if (mode == WIFI_MODE_STA) return "STA";
  if (mode == WIFI_MODE_APSTA) return "AP+STA";
  return "NONE";
}
} // namespace

WebServerManager::WebServerManager(NvsManager &nvsManager)
    : nvsManager_(nvsManager), server_(Config::WEB_SERVER_PORT), started_(false), rebootScheduled_(false), rebootScheduleMs_(0) {}

void WebServerManager::begin() {
  if (started_) return;

server_.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    String ssid, password, ipAddress, gateway, subnet;
    (void)nvsManager_.getWifiCredentials(ssid, password);
    if (!nvsManager_.getStaNetworkConfig(ipAddress, gateway, subnet)) {
      ipAddress = Config::WIFI_STA_DEFAULT_STATIC_IP;
      gateway = Config::WIFI_STA_DEFAULT_GATEWAY;
      subnet = Config::WIFI_STA_DEFAULT_SUBNET;
    }
    String currentIp = WiFi.localIP().toString();
    if (currentIp == "0.0.0.0") currentIp = WiFi.softAPIP().toString();

    // 1. HTML değişkenini tanımla
    String html = R"rawliteral(
<!doctype html>
<html lang="tr">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>TeosSensor Network Console</title>
  <link href="https://fonts.googleapis.com/css2?family=Space+Grotesk:wght@400;500;700&display=swap" rel="stylesheet">
  <style>
    :root{--bg:#f3f5f8;--ink:#0f172a;--muted:#475569;--card:#ffffff;--line:#dbe2ea;--brand:#0e7490;--brand-2:#0f766e;--danger:#be123c;--accent:#b45309;--ok:#166534;--shadow:0 14px 32px rgba(15,23,42,.08);}
    *{box-sizing:border-box} body{margin:0;font-family:'Space Grotesk',sans-serif;color:var(--ink);background:var(--bg);}
    .wrap{max-width:980px;margin:34px auto;padding:0 16px} .hero{display:flex;justify-content:space-between;flex-wrap:wrap;margin-bottom:14px}
    .title{font-size:1.8rem;margin:0} .sub{margin:6px 0 0;color:var(--muted)} .grid{display:grid;grid-template-columns:1fr;gap:14px}
    @media (min-width:860px){.grid{grid-template-columns:2fr 1fr}} .card{background:var(--card);border:1px solid var(--line);border-radius:20px;padding:18px;}
    .row{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:10px} label{display:block;font-size:.86rem;margin-bottom:4px}
    input{width:100%;border:1px solid var(--line);border-radius:10px;padding:10px;} button{border:0;border-radius:12px;padding:10px;cursor:pointer;font-weight:bold;}
    .primary{background:var(--brand);color:#fff} .danger{background:#fff;color:var(--danger);border:1px solid #fecdd3}
    .secondary{background:#fff;color:var(--accent);border:1px solid #fde68a} .stat{display:flex;justify-content:space-between;padding:9px 0;border-bottom:1px dashed var(--line);}
    #result{margin-top:12px;padding:10px;border-radius:10px;display:none} .ok{display:block;background:#dcfce7;color:var(--ok)} .err{display:block;background:#fee2e2;color:#991b1b}
  </style>
</head>
<body>
  <div class="wrap">
    <div class="hero"><div><h1 class="title">TeosSensor Console</h1><p class="sub">Wi-Fi + statik ag ayarlari.</p></div></div>
    <div class="grid">
      <div class="card">
        <form id="networkForm">
          <div class="row"><div><label>SSID</label><input name="ssid" required value="__SSID__"></div><div><label>Sifre</label><input name="password" type="password"></div></div>
          <div class="row"><div><label>IP</label><input name="ip" value="__IP__" required></div><div><label>Gateway</label><input name="gateway" value="__GW__" required></div></div>
          <div class="row"><div><label>Subnet</label><input name="subnet" value="__MASK__" required></div></div>
          <button class="primary" type="submit">Kaydet ve Yeniden Baslat</button>
          <div id="result"></div>
        </form>
      </div>
      <div class="card">
        <div class="stat"><span>Mod</span><strong>__MODE__</strong></div>
        <div class="stat"><span>Aktif IP</span><strong>__CURRENT_IP__</strong></div>
        <div class="stat"><span>Son Kart</span><strong id="lastUidValue">__LAST_UID__</strong></div>
        <form id="factoryForm" style="margin-top:12px"><button class="danger" type="submit">Factory Reset</button></form>
        <form id="restartForm" style="margin-top:10px"><button class="secondary" type="submit">Restart Device</button></form>
      </div>
    </div>
  </div>
  <script>
    const res = document.getElementById('result');
    function show(k, t){ res.className=k; res.textContent=t; }
    document.getElementById('networkForm').addEventListener('submit', async(e)=>{
      e.preventDefault(); const f=new URLSearchParams(new FormData(e.target));
      const r=await fetch('/set-wifi',{method:'POST',body:f}); const d=await r.json(); show(r.ok?'ok':'err',d.message||d.error);
    });
    document.getElementById('factoryForm').addEventListener('submit', async(e)=>{
      e.preventDefault(); if(confirm('Silinsin mi?')){ const r=await fetch('/factory-reset',{method:'POST'}); const d=await r.json(); show(r.ok?'ok':'err',d.message||d.error); }
    });
    document.getElementById('restartForm').addEventListener('submit', async(e)=>{
      e.preventDefault(); if(confirm('Baslat?')){ const r=await fetch('/restart',{method:'POST'}); const d=await r.json(); show(r.ok?'ok':'err',d.message||d.error); }
    });
  </script>
</body></html>
)rawliteral";

    // 2. Yer tutucuları doldur
    uint32_t lastUid;
    html.replace("__LAST_UID__", nvsManager_.getLastUid(lastUid) ? formatUidHex(lastUid) : "N/A");
    html.replace("__MODE__", wifiModeLabel());
    html.replace("__SSID__", escapeHtml(ssid));
    html.replace("__IP__", escapeHtml(ipAddress));
    html.replace("__GW__", escapeHtml(gateway));
    html.replace("__MASK__", escapeHtml(subnet));
    html.replace("__CURRENT_IP__", escapeHtml(currentIp));
    
    request->send(200, "text/html", html);
  });


  server_.on("/set-wifi", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (!request->hasParam("ssid", true)) { request->send(400, "application/json", "{\"ok\":false,\"error\":\"Missing ssid\"}"); return; }
    String ssid = request->getParam("ssid", true)->value();
    String password = request->hasParam("password", true) ? request->getParam("password", true)->value() : "";
    String staticIp = request->hasParam("ip", true) ? request->getParam("ip", true)->value() : Config::WIFI_STA_DEFAULT_STATIC_IP;
    String gateway = request->hasParam("gateway", true) ? request->getParam("gateway", true)->value() : Config::WIFI_STA_DEFAULT_GATEWAY;
    String subnet = request->hasParam("subnet", true) ? request->getParam("subnet", true)->value() : Config::WIFI_STA_DEFAULT_SUBNET;
    
    if (password.length() == 0) {
      String storedSsid, storedPassword;
      if (nvsManager_.getWifiCredentials(storedSsid, storedPassword) && storedSsid == ssid) password = storedPassword;
    }
    if (!nvsManager_.saveWifiCredentials(ssid, password) || !nvsManager_.saveStaNetworkConfig(staticIp, gateway, subnet)) {
      request->send(500, "application/json", "{\"ok\":false,\"error\":\"NVS write failed\"}"); return;
    }
    request->send(200, "application/json", "{\"ok\":true,\"message\":\"Saved. Rebooting...\"}");
    scheduleReboot_();
  });

  server_.on("/factory-reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (!nvsManager_.factoryReset()) { request->send(500, "application/json", "{\"ok\":false,\"error\":\"Factory reset failed\"}"); return; }
    request->send(200, "application/json", "{\"ok\":true,\"message\":\"Reset complete. Rebooting...\"}");
    scheduleReboot_();
  });

  server_.on("/restart", HTTP_POST, [this](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"ok\":true,\"message\":\"Rebooting...\"}");
    scheduleReboot_();
  });

  server_.begin();
  started_ = true;
  Serial.println("[WEB] Web server started.");
}

void WebServerManager::loop() {
  if (rebootScheduled_ && (millis() - rebootScheduleMs_) >= Config::WEB_REBOOT_DELAY_MS) {
    Serial.println("[WEB] Rebooting now...");
    ESP.restart();
  }
}

void WebServerManager::scheduleReboot_() {
  rebootScheduled_ = true;
  rebootScheduleMs_ = millis();
}