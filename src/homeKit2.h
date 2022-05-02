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
    uint8_t getConnectedClientsCount();

private:
    homeKit2(){};
    static void updateChaValue(homekit_characteristic_t &cha, float value);
    static void updateChaValue(homekit_characteristic_t &cha, const char* value);
    static void updateChaValue(homekit_characteristic_t &cha, uint64_t value);
    static void updateChaValue(homekit_characteristic_t &cha, int value);
    static void updatePassword(const char *password);

    void notifyTemperatureChange();
    void notifyHumidityChange();
    void notifySensorRefreshIntervalChange();
    void notifyIPAddressChange();
    void notifyWifiRssiChange();
    void updateAccessoryName();

    void onConfigChange();
    static void onSensorRefreshIntervalChange(const homekit_value_t);

    String accessoryName;
    String password;
    String serialNumber;
    String localIP;
    int rssi;
    bool notifyTemp;
    uint64_t sensorRefreshInterval;
    uint64_t lastCheckedForNonEvents{0};
};

