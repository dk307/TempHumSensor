#pragma once

#include <homekit/homekit.h>
#include <homekit/characteristics.h>

class homeKit2
{
public:
    void begin();
    void loop();
    bool isPaired();

    static homeKit2 instance;

    const String &getPassword()
    {
        return password;
    }

private:
    homeKit2(){};
    static void updateChaValues();
    static void updateChaValue(homekit_characteristic_t &cha, float value);
    static void updateChaValue(homekit_characteristic_t &cha, const char* value);
    static void updatePassword(const char *password);

    void notifyTemperatureChange();
    void notifyHumidityChange();

    String accessoryName;
    String password;
    String serialNumber;
    bool notifyTemp;
};

