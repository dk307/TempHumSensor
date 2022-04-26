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
    bool startUpdate(size_t length, String &error);
    bool writeUpdate(const uint8_t *data, size_t length, String &error);
    bool endUpdate(String &error);

    static operations instance;

private:
    operations() {}
    bool beginFS();
    void getUpdateError(String &error);
    [[noreturn]] static void reset();

    MultiResetDetector *mrd = nullptr;

    bool rebootPending{false};
};
