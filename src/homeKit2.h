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

private:
    homeKit2(){};
    static void storageChanged(char *szstorage, int bufsize);
    static void updatePassword(const char *password);
    String accessoryName;
    String password;
};

extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t chaCurrentTemperature;
extern "C" homekit_characteristic_t chaHumidity;
