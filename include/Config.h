#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

namespace Config {
    // --- Wi-Fi Zamanlama ve Varsayılan Ağ Ayarları ---
    constexpr uint32_t WIFI_RECONNECT_INTERVAL_MS = 5000;
    constexpr uint32_t WIFI_STA_CONNECT_TIMEOUT_MS = 10000; 
    
    // --- Access Point (AP) Modu Ayarları ---
    const char* const AP_SSID = "TeosSensor_AP"; 
    const char* const AP_PASSWORD = "admin123";     

    const char* const WIFI_STA_DEFAULT_STATIC_IP = "192.168.1.100";
    const char* const WIFI_STA_DEFAULT_GATEWAY   = "192.168.1.1";
    const char* const WIFI_STA_DEFAULT_SUBNET    = "255.255.255.0";

    // --- Web Server Ayarları ---
    constexpr uint16_t WEB_SERVER_PORT = 80;       
    constexpr uint32_t WEB_REBOOT_DELAY_MS = 2000; 

    // --- Modbus TCP Ayarları ---
    constexpr uint16_t MODBUS_TCP_PORT = 502;
    constexpr uint16_t MODBUS_HREG_UID_HIGH = 0; 
    constexpr uint16_t MODBUS_HREG_UID_LOW  = 1; 
    // Son Okunan Kart Registerları
    constexpr uint16_t MODBUS_HREG_LAST_UID_HIGH = 2; 
    constexpr uint16_t MODBUS_HREG_LAST_UID_LOW  = 3;

    // --- RFID PN532 I2C ve Donanım Pin Ayarları ---
    constexpr uint8_t RFID_SDA_PIN         = 1; 
    constexpr uint8_t RFID_SCL_PIN         = 2;
    constexpr uint8_t RFID_PN532_IRQ_PIN   = 11;
    constexpr uint8_t RFID_PN532_RESET_PIN = 12;
    constexpr uint32_t I2C_CLOCK_SPEED     = 50000; 
    constexpr uint8_t RFID_NTAG_FIRST_USER_PAGE = 4;
    constexpr uint8_t RFID_NTAG_MAX_READ_PAGES = 48;
    constexpr uint16_t RFID_NTAG_MAX_READ_BYTES = RFID_NTAG_MAX_READ_PAGES * 4;

    // --- Donanım Gösterge Pinleri ---
    // ESP32-S3-Zero üzerindeki BOOT düğmesi GPIO0'ı GND'ye çeker.
    constexpr uint8_t BOOT_BUTTON_PIN = 0;
    constexpr uint8_t RFID_IS_READ_PIN = 3;
    constexpr uint8_t LED_AP_PIN = 5;
    constexpr uint8_t LED_STA_PIN = 6;
    constexpr uint8_t BUZZER_PIN = 7;

    // --- Donanım Buton Zamanlamaları ---
    constexpr uint32_t BOOT_BUTTON_DEBOUNCE_MS = 50;

    // --- NVS (Kalıcı Hafıza) Namespace ve Anahtarları ---
    const char* const NVS_NAMESPACE          = "teos_storage";
    const char* const NVS_KEY_WIFI_SSID      = "w_ssid";
    const char* const NVS_KEY_WIFI_PASS      = "w_pass";
    const char* const NVS_KEY_STA_STATIC_IP  = "sta_ip";
    const char* const NVS_KEY_STA_GATEWAY    = "sta_gw";
    const char* const NVS_KEY_STA_SUBNET     = "sta_sn";
    const char* const NVS_KEY_LAST_UID       = "last_uid";

    // --- Super-Loop Zamanlama Ayarları ---
    constexpr uint32_t NETWORK_TASK_DELAY_MS = 100;
    constexpr uint32_t RFID_TASK_DELAY_MS    = 50; 
    constexpr uint32_t RFID_UID_CLEAR_TIMEOUT_MS = 5000;
}

#endif // CONFIG_H
