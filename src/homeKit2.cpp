#include <Arduino.h>

#include "homeKit2.h"
#include "configManager.h"
#include "hardware.h"
#include "logging.h"

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

    LOG_INFO(F("HomeKit Server Running"));
}

void homeKit2::loop()
{
    arduino_homekit_loop();
}

bool homeKit2::isPaired()
{
    auto server = arduino_homekit_get_running_server();
    if (server)
    {
        return server->paired;
    }
    return false;
}

void homeKit2::updatePassword(const char *password)
{
    homeKit2::instance.password = password;
}

bool read_storage(uint32 srcAddress, byte *desAddress, uint32 size)
{
    const auto &data = config::instance.data.homeKitPairData;

    if (data.size() > srcAddress + size)
    {
        return false;
    }
    memcpy(desAddress, data.data() + srcAddress, size);
    return true;
}

bool write_storage(uint32 desAddress, byte *srcAddress, uint32 size)
{
    auto &data = config::instance.data.homeKitPairData;
    if (desAddress + size < data.size())
    {
        data.resize(desAddress + size);
    }

    memcpy(data.data() + desAddress, srcAddress, size);
    config::instance.save();
    return true;
}

bool reset_storage()
{
    config::instance.data.homeKitPairData.empty();
    config::instance.save();
    return true;
}
