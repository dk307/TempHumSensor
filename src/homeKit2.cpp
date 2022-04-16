#include <Arduino.h>

#include "homeKit2.h"
#include "configManager.h"
#include "hardware.h"

#include <arduino_homekit_server.h>

homeKit2 homeKit2::instance;

void homeKit2::begin()
{
    chaCurrentTemperature.value.float_value = hardware::instance.getTemperatureC();
    chaHumidity.value.float_value = hardware::instance.getHumidity();

    accessoryName = config::instance.data.hostName;

    config.password_callback = &homeKit2::updatePassword;

    if (!accessoryName.isEmpty())
    {
        config.accessories[0]->services[0]->characteristics[0]->value.string_value =
            const_cast<char *>(accessoryName.c_str());
    }

    arduino_homekit_setup(&config);
}

void homeKit2::loop()
{
    arduino_homekit_loop();
}

bool homeKit2::isPaired()
{
    auto server = arduino_homekit_get_running_server();
    if (server) {
        return server->paired;
    }
    return false;
}

void homeKit2::storageChanged(char *szstorage, int bufsize)
{
}

void homeKit2::updatePassword(const char *password)
{
    homeKit2::instance.password = password;
}