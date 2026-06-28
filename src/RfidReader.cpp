#include "RfidReader.h"
#include "Config.h"

namespace {
bool isConfigTextMarker(char value) {
    return value == '{' || value == 's' || value == 'S';
}

bool isReadableTextByte(uint8_t value) {
    return value == '\n' || value == '\r' || value == '\t' || (value >= 32 && value <= 126);
}
} // namespace

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

    String currentPayload;
    if (!readConfigTextFromTag_(currentPayload)) {
        return false;
    }

    // Minimum uzunluk kontrolü (çöp verileri engellemek için)
    if (currentPayload.length() < 10) {
        return false;
    }

    configData = currentPayload;
    Serial.printf("[RFID] NFC configuration payload: %s\n", configData.c_str());
    return true;
}

bool RfidReader::readConfigTextFromTag_(String &configData) {
    uint8_t tagData[Config::RFID_NTAG_MAX_READ_BYTES] = {0};
    uint16_t tagLength = 0;
    bool foundTerminator = false;

    for (uint8_t pageOffset = 0; pageOffset < Config::RFID_NTAG_MAX_READ_PAGES; ++pageOffset) {
        uint8_t pageData[4] = {0};
        const uint8_t page = Config::RFID_NTAG_FIRST_USER_PAGE + pageOffset;
        
        // Eğer okuma başarısız olursa, KISMİ VERİYİ KABUL ETME, DİREKT İPTAL ET
        if (!pn532_.ntag2xx_ReadPage(page, pageData)) {
            Serial.println("[RFID] Read page failed, aborting partial read.");
            return false;
        }

        for (uint8_t byteIndex = 0; byteIndex < sizeof(pageData); ++byteIndex) {
            if (tagLength >= Config::RFID_NTAG_MAX_READ_BYTES) break;
            tagData[tagLength++] = pageData[byteIndex];
        }

        for (uint8_t byteIndex = 0; byteIndex < sizeof(pageData); ++byteIndex) {
            if (pageData[byteIndex] == 0xFE) {
                foundTerminator = true;
                break;
            }
        }
        if (foundTerminator) break;
    }

    if (tagLength == 0) return false;
    if (extractNdefTextFromTag_(tagData, tagLength, configData)) return true;
    return extractPlainTextFromTag_(tagData, tagLength, configData);
}

bool RfidReader::extractNdefTextFromTag_(const uint8_t *tagData, uint16_t tagLength, String &configData) const {
    uint16_t index = 0;

    while (index < tagLength) {
        const uint8_t tlvType = tagData[index++];
        if (tlvType == 0x00) continue;
        if (tlvType == 0xFE) break;
        if (index >= tagLength) return false;

        uint32_t tlvLength = tagData[index++];
        if (tlvLength == 0xFF) {
            if (index + 1 >= tagLength) return false;
            tlvLength = (static_cast<uint32_t>(tagData[index]) << 8) | tagData[index + 1];
            index += 2;
        }

        if (index + tlvLength > tagLength) return false;
        if (tlvType == 0x03 && extractTextRecord_(tagData + index, static_cast<uint16_t>(tlvLength), configData)) {
            return true;
        }
        index += tlvLength;
    }

    return false;
}

bool RfidReader::extractTextRecord_(const uint8_t *message, uint16_t messageLength, String &configData) const {
    uint16_t index = 0;

    while (index < messageLength) {
        if (index + 2 > messageLength) return false;

        const uint8_t header = message[index++];
        const bool shortRecord = (header & 0x10) != 0;
        const bool idLengthPresent = (header & 0x08) != 0;
        const uint8_t typeNameFormat = header & 0x07;
        const uint8_t typeLength = message[index++];

        uint32_t payloadLength = 0;
        if (shortRecord) {
            if (index >= messageLength) return false;
            payloadLength = message[index++];
        } else {
            if (index + 3 >= messageLength) return false;
            payloadLength = (static_cast<uint32_t>(message[index]) << 24) |
                            (static_cast<uint32_t>(message[index + 1]) << 16) |
                            (static_cast<uint32_t>(message[index + 2]) << 8) |
                            message[index + 3];
            index += 4;
        }

        uint8_t idLength = 0;
        if (idLengthPresent) {
            if (index >= messageLength) return false;
            idLength = message[index++];
        }

        if (index + typeLength + idLength + payloadLength > messageLength) return false;

        const uint8_t *type = message + index;
        index += typeLength + idLength;
        const uint8_t *payload = message + index;
        index += payloadLength;

        const bool isTextRecord = typeNameFormat == 0x01 && typeLength == 1 && type[0] == 'T';
        const bool isTextMimeRecord = typeNameFormat == 0x02 && typeLength >= 5 &&
                                      type[0] == 't' && type[1] == 'e' && type[2] == 'x' &&
                                      type[3] == 't' && type[4] == '/';

        if (isTextRecord && payloadLength > 1) {
            const uint8_t status = payload[0];
            const uint8_t languageLength = status & 0x3F;
            const bool utf16Encoded = (status & 0x80) != 0;
            if (!utf16Encoded && payloadLength > static_cast<uint32_t>(1 + languageLength)) {
                const uint16_t textStart = 1 + languageLength;
                const uint16_t textLength = static_cast<uint16_t>(payloadLength - textStart);
                configData = "";
                configData.reserve(textLength);
                for (uint16_t byteIndex = 0; byteIndex < textLength; ++byteIndex) {
                    configData += static_cast<char>(payload[textStart + byteIndex]);
                }
                configData.trim();
                return configData.length() > 0;
            }
        }

        if (isTextMimeRecord && payloadLength > 0) {
            configData = "";
            configData.reserve(payloadLength);
            for (uint32_t byteIndex = 0; byteIndex < payloadLength; ++byteIndex) {
                configData += static_cast<char>(payload[byteIndex]);
            }
            configData.trim();
            return configData.length() > 0;
        }

        if ((header & 0x40) != 0) break;
    }

    return false;
}

bool RfidReader::extractPlainTextFromTag_(const uint8_t *tagData, uint16_t tagLength, String &configData) const {
    String text;
    text.reserve(tagLength);

    for (uint16_t byteIndex = 0; byteIndex < tagLength; ++byteIndex) {
        const uint8_t value = tagData[byteIndex];
        if (value == 0xFE) break;
        if (isReadableTextByte(value)) text += static_cast<char>(value);
    }

    text.trim();
    if (text.length() == 0) return false;

    int startIndex = text.indexOf('{');
    if (startIndex < 0) {
        for (uint16_t charIndex = 0; charIndex < text.length(); ++charIndex) {
            if (isConfigTextMarker(text[charIndex])) {
                startIndex = charIndex;
                break;
            }
        }
    }

    if (startIndex > 0) text = text.substring(startIndex);
    text.trim();
    if (text.length() == 0) return false;

    configData = text;
    return true;
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
