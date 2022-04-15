#pragma once

class MultiResetDetector;

class operations
{
public:
    void reboot();
    void begin();
    void loop();

    void factoryReset();

    static operations instance;

private:
    operations() {}
    MultiResetDetector *mrd = nullptr;

    bool rebootPending{false};
};
