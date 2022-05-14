#include <Arduino.h>

#include "homeKit2.h"
#include "configManager.h"
#include "WiFiManager.h"
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
extern "C" homekit_characteristic_t chaWifiIPAddress;
extern "C" homekit_characteristic_t chaWifiRssi;
extern "C" homekit_characteristic_t chaSensorRefershInterval;

#define FIRST_ACCESSORY 0
#define INFO_SERVICE 0
#define NAME_CHA 0
#define SERIAL_NUMBER_CHA 2

void homeKit2::begin()
{
    config.password_callback = &homeKit2::updatePassword;

    serialNumber = String(ESP.getChipId(), HEX);
    serialNumber.toUpperCase();

    config::instance.addConfigSaveCallback(std::bind(&homeKit2::onConfigChange, this));
    hardware::instance.temperatureChangeCallback.addConfigSaveCallback(std::bind(&homeKit2::notifyTemperatureChange, this));
    hardware::instance.humidityChangeCallback.addConfigSaveCallback(std::bind(&homeKit2::notifyHumidityChange, this));
    chaSensorRefershInterval.setter = onSensorRefreshIntervalChange;

    updateChaValue(*config.accessories[FIRST_ACCESSORY]->services[INFO_SERVICE]->characteristics[SERIAL_NUMBER_CHA], serialNumber.c_str());
    notifyIPAddressChange();
    notifyWifiRssiChange();
    notifyTemperatureChange();
    notifyHumidityChange();
    notifySensorRefreshIntervalChange();
    updateAccessoryName();

    localIP = WifiManager::instance.LocalIP().toString();
    rssi = WifiManager::instance.RSSI();

    arduino_homekit_setup(&config);

    LOG_INFO(F("HomeKit Server Running"));

    notifyTemperatureChange();
    notifyHumidityChange();
    notifySensorRefreshIntervalChange();
    notifyIPAddressChange();
    notifyWifiRssiChange();
}

void homeKit2::onConfigChange()
{
    if ((accessoryName != config::instance.data.hostName) && (!config::instance.data.hostName.isEmpty()))
    {
        updateAccessoryName();
    }

    if (sensorRefreshInterval != (config::instance.data.sensorsRefreshInterval / 1000))
    {
        notifySensorRefreshIntervalChange();
    }
}

void homeKit2::updateAccessoryName()
{
    accessoryName = config::instance.data.hostName;
    if (accessoryName.isEmpty())
    {
        accessoryName = F("Sensor");
    }
    updateChaValue(*config.accessories[FIRST_ACCESSORY]->services[INFO_SERVICE]->characteristics[NAME_CHA], accessoryName.c_str());
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

void homeKit2::notifySensorRefreshIntervalChange()
{
    sensorRefreshInterval = config::instance.data.sensorsRefreshInterval / 1000;
    updateChaValue(chaSensorRefershInterval, sensorRefreshInterval);
    homekit_characteristic_notify(&chaSensorRefershInterval, chaSensorRefershInterval.value);
}

void homeKit2::notifyIPAddressChange()
{
    localIP = WifiManager::instance.LocalIP().toString();
    updateChaValue(chaWifiIPAddress, localIP.c_str());
    homekit_characteristic_notify(&chaWifiIPAddress, chaWifiIPAddress.value);
}

void homeKit2::notifyWifiRssiChange()
{
    rssi = WifiManager::instance.RSSI();
    updateChaValue(chaWifiRssi, rssi);
    homekit_characteristic_notify(&chaWifiRssi, chaWifiRssi.value);
}

void homeKit2::loop()
{
    const auto now = millis();
    if ((now - lastCheckedForNonEvents > config::instance.data.sensorsRefreshInterval))
    {
        if (localIP != WifiManager::instance.LocalIP().toString())
        {
            notifyIPAddressChange();
        }
        const auto delta = std::abs(rssi - WifiManager::instance.RSSI());
        if (delta > 2)
        {
            notifyWifiRssiChange();
        }
        lastCheckedForNonEvents = now;
    }

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

void homeKit2::updateChaValue(homekit_characteristic_t &cha, uint64_t value)
{
    cha.value.uint64_value = value;
}

void homeKit2::updateChaValue(homekit_characteristic_t &cha, int value)
{
    cha.value.int_value = value;
}

void homeKit2::onSensorRefreshIntervalChange(const homekit_value_t value)
{
    if (value.format == homekit_format_uint64)
    {
        homeKit2::instance.instance.sensorRefreshInterval = value.uint64_value;
        updateChaValue(chaSensorRefershInterval, value.uint64_value);
        config::instance.data.sensorsRefreshInterval = homeKit2::instance.instance.sensorRefreshInterval * 1000;
        config::instance.save();
    }
}

uint8_t homeKit2::getConnectedClientsCount()
{
    return arduino_homekit_connected_clients_count();
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
