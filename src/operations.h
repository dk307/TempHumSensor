#pragma once

#include <Arduino.h>

class MultiResetDetector;

class operations
{
public:
    void reboot();
    void begin();
    void loop();

    void factoryReset();

    //update
    bool startUpdate(size_t length, const String& md5, String &error);
    bool writeUpdate(const uint8_t *data, size_t length, String &error);
    bool endUpdate(String &error);
    void abortUpdate();
    bool isUpdateInProgress();

    static operations instance;

private:
    operations() {}
    bool beginFS();
    void getUpdateError(String &error);
    [[noreturn]] static void reset();

    MultiResetDetector *mrd = nullptr;

    bool rebootPending{false};
};
