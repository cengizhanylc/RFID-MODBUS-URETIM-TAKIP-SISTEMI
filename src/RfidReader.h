#ifndef RFID_READER_H
#define RFID_READER_H

#include <Wire.h>
#include <Adafruit_PN532.h>
#include <Arduino.h>

enum class CardResult {
    NONE,           
    NEW_UID,        
    CONFIG_TAG      
};

class RfidReader {
public:
    RfidReader();
    bool begin();
    
    // YENİ: checkNdef parametresi eklendi
    CardResult scanForCard(uint32_t &uid, String &configData, bool checkNdef);
    
    void setLastSeenUid(uint32_t uid);

private:
    Adafruit_PN532 pn532_;
    bool initialized_;
    bool hasLastUid_;
    uint32_t lastUid_;
    bool cardPresent_;
    uint8_t lastRawUid_[7];
    uint8_t lastRawUidLength_;

    uint32_t mapUidToUint32_(const uint8_t *uidBytes, uint8_t uidLength) const;
    bool checkForConfigTag_(uint8_t *uidBytes, uint8_t uidLength, String &configData);
};

#endif // RFID_READER_H