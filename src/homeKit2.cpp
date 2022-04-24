#include <Arduino.h>

#include "homeKit2.h"
#include "configManager.h"
#include "hardware.h"
#include "logging.h"

#include <math.h>
#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <arduino_homekit_server.h>

homeKit2 homeKit2::instance;

extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t chaCurrentTemperature;
extern "C" homekit_characteristic_t chaHumidity;

void homeKit2::begin()
{
    config.password_callback = &homeKit2::updatePassword;   
    serialNumber = String(ESP.getChipId(), HEX);
    serialNumber.toUpperCase();

    updateChaValues();
    updateChaValue(*config.accessories[0]->services[0]->characteristics[2], serialNumber.c_str());

    accessoryName = config::instance.data.hostName;
    config.password_callback = &homeKit2::updatePassword;

    config::instance.addConfigSaveCallback([this]
                                           {                                      
        if ((accessoryName != config::instance.data.hostName) && (!config::instance.data.hostName.isEmpty()))
        {
            accessoryName = config::instance.data.hostName;
            updateChaValue(*config.accessories[0]->services[0]->characteristics[0], accessoryName.c_str());
        } });

    if (!accessoryName.isEmpty())
    {
       
        updateChaValue(*config.accessories[0]->services[0]->characteristics[0], accessoryName.c_str());
    }

    arduino_homekit_setup(&config);

    LOG_INFO(F("HomeKit Server Running"));

    hardware::instance.temperatureChangeCallback.addConfigSaveCallback(std::bind(&homeKit2::notifyTemperatureChange, this));
    hardware::instance.humidityChangeCallback.addConfigSaveCallback(std::bind(&homeKit2::notifyHumidityChange, this));

    notifyTemperatureChange();
    notifyHumidityChange();
}

void homeKit2::notifyTemperatureChange()
{
    updateChaValue(chaCurrentTemperature, hardware::instance.getTemperatureC());
    homekit_characteristic_notify(&chaCurrentTemperature, chaCurrentTemperature.value);
}

void homeKit2::notifyHumidityChange()
{
    updateChaValue(chaHumidity, hardware::instance.getHumidity());
    homekit_characteristic_notify(&chaHumidity, chaHumidity.value);
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

void homeKit2::updateChaValue(homekit_characteristic_t &cha, float value)
{
    LOG_TRACE(F("Name:") << value);
    if (!isnan(value))
    {
        cha.value.is_null = false;
        cha.value.float_value = value;
    }
    else
    {
        cha.value.is_null = true;
    }
}

void homeKit2::updateChaValue(homekit_characteristic_t &cha, const char *value)
{
     LOG_INFO(F("Name:") << value);
    if (value)
    {
        cha.value.is_null = false;
        cha.value.string_value = const_cast<char *>(value);
    }
    else
    {
        cha.value.is_null = true;
    }
}

void homeKit2::updateChaValues()
{
    updateChaValue(chaCurrentTemperature, hardware::instance.getTemperatureC());
    updateChaValue(chaHumidity, hardware::instance.getHumidity());
}

bool read_storage(uint32 srcAddress, byte *desAddress, uint32 size)
{
    LOG_TRACE(F("Reading HomeKit data from ") << srcAddress << F(" size ") << size);
    const auto &data = config::instance.data.homeKitPairData;
    if (data.size() < (srcAddress + size))
    {
        LOG_TRACE(F("Returning empty for ") << srcAddress << F(" size ") << size);
        memset(desAddress, 0xFF, size);
    }
    else
    {
        memcpy(desAddress, data.data() + srcAddress, size);
    }
    return true;
}

bool write_storage(uint32 desAddress, byte *srcAddress, uint32 size)
{
    LOG_TRACE(F("Writing HomeKit data at ") << desAddress << F(" size ") << size);
    auto &data = config::instance.data.homeKitPairData;
    if ((desAddress + size) > data.size())
    {
        data.resize(desAddress + size);
    }

    memcpy(data.data() + desAddress, srcAddress, size);
    config::instance.save();
    return true;
}

bool reset_storage()
{
    config::instance.data.homeKitPairData.resize(0);
    config::instance.save();
    return true;
}
