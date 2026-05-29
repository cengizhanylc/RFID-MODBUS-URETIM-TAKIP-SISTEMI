#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "NvsManager.h"

class WebServerManager {
public:
    explicit WebServerManager(NvsManager &nvsManager);
    void begin();
    void loop();

private:
    void scheduleReboot_();

    NvsManager &nvsManager_;
    AsyncWebServer server_;
    bool started_;
    bool rebootScheduled_;
    uint32_t rebootScheduleMs_;
};

#endif // WEB_SERVER_MANAGER_H