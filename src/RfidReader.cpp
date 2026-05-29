#include "RfidReader.h"
#include "Config.h"

RfidReader::RfidReader()
    : pn532_(Config::RFID_PN532_IRQ_PIN, Config::RFID_PN532_RESET_PIN),
      initialized_(false), hasLastUid_(false), lastUid_(0), cardPresent_(false),
      lastRawUid_{0}, lastRawUidLength_(0) {}

bool RfidReader::begin() {
    if (initialized_) return true;
    Wire.end(); 
    Wire.begin(Config::RFID_SDA_PIN, Config::RFID_SCL_PIN);
    Wire.setClock(Config::I2C_CLOCK_SPEED); // Hızı garanti altına aldık
    pn532_.begin();

    uint32_t versionData = pn532_.getFirmwareVersion();
    if (!versionData) return false;
    pn532_.SAMConfig();
    initialized_ = true;
    return true;
}

CardResult RfidReader::scanForCard(uint32_t &uid, String &configData, bool isApMode) {
    if (!initialized_) return CardResult::NONE;
    uint8_t uidBuffer[7] = {0};
    uint8_t uidLength = 0;
    
    if (pn532_.readPassiveTargetID(PN532_MIFARE_ISO14443A, uidBuffer, &uidLength, 40)) {
        cardPresent_ = true; 

        // AP MODUNDA SADECE NDEF ARA
        if (isApMode) {
            if (checkForConfigTag_(uidBuffer, uidLength, configData)) {
                return CardResult::CONFIG_TAG; 
            }
            return CardResult::NONE; 
        }

        // STA MODUNDA SADECE RFID ARA
        uint32_t parsedUid = mapUidToUint32_(uidBuffer, uidLength);
        if (parsedUid != lastUid_) {
            lastUid_ = parsedUid;
            uid = parsedUid;
            hasLastUid_ = true; 
            memcpy(lastRawUid_, uidBuffer, uidLength); 
            lastRawUidLength_ = uidLength; 
            return CardResult::NEW_UID; 
        }
    } else {
        cardPresent_ = false; 
    }
    return CardResult::NONE; 
}

bool RfidReader::checkForConfigTag_(uint8_t *uidBytes, uint8_t uidLength, String &configData) {
    if (uidLength != 7) return false; // NTAG2xx etiketleri genelde 7 byte UID'dir

    uint8_t data[32];
    
    // 4. sayfadan başlayarak 10. sayfaya kadar tara (Çöp karakteri atlayıp "{" işaretini ara)
    for (int page = 4; page < 10; page++) {
        if (pn532_.ntag2xx_ReadPage(page, data)) {
            // Veri '{' karakteri ile başlıyorsa bu bizim JSON'umuzdur
            if (data[0] == '{') {
                configData = String((char*)data);
                
                // Eğer JSON'un devamı diğer sayfalardaysa onu da almamız gerekebilir
                // Ama ilk etapta sadece 16 byte yeterli, bakalım yakalayacak mı?
                Serial.printf("[RFID] Konfigurasyon bulundu! Sayfa: %d, Veri: %s\n", page, configData.c_str());
                return true; 
            }
        }
    }
    return false; 
}

void RfidReader::setLastSeenUid(uint32_t uid) {
    lastUid_ = uid;
    hasLastUid_ = true;
}

uint32_t RfidReader::mapUidToUint32_(const uint8_t *uidBytes, uint8_t uidLength) const {
    if (uidLength == 4) {
        return (uidBytes[0] << 24) | (uidBytes[1] << 16) | (uidBytes[2] << 8) | uidBytes[3];
    }
    uint32_t hash = 2166136261UL;
    for (uint8_t i = 0; i < uidLength; ++i) {
        hash ^= uidBytes[i];
        hash *= 16777619UL;
    }
    return hash;
}