#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Hash.h>

#include "configManager.h"

static const char ConfigFilePath[] PROGMEM = "/Config.json";
static const char ConfigChecksumFilePath[] PROGMEM = "/ConfigChecksum.json";
static const char HostNameId[] PROGMEM = "hostname";
static const char WebUserNameId[] PROGMEM = "webusername";
static const char WebPasswordId[] PROGMEM = "webpassword";
static const char HomeKitPairDataId[] PROGMEM = "homekitpairdata";
static const char SensorsRefreshIntervalId[] PROGMEM = "sensorsrefreshinterval";
static const char ShowDisplayInFId[] PROGMEM = "showdisplayinf";

config config::instance;

void config::erase()
{
    LittleFS.remove(FPSTR(ConfigChecksumFilePath));
    LittleFS.remove(FPSTR(ConfigFilePath));
}

bool config::begin()
{
    const auto json = readFromFile(ConfigFilePath);
    if (json.isEmpty())
    {
        Serial.println(PSTR("No stored config found"));
        reset();
        return false;
    }

    const auto readChecksum = readFromFile(ConfigChecksumFilePath);
    const auto checksum = sha1(json);

    if (checksum != readChecksum)
    {
        Serial.println(PSTR("Config data checksum mismatch"));
        reset();
        return false;
    }

    DynamicJsonDocument jsonBuffer(1024);
    const DeserializationError error = deserializeJson(jsonBuffer, json);

    // Test if parsing succeeds.
    if (error)
    {
        Serial.print(F("deserializeJson for config failed: "));
        Serial.println(error.f_str());
        reset();
        return false;
    }

    data.hostName = jsonBuffer[FPSTR(HostNameId)].as<String>();
    data.webUserName = jsonBuffer[FPSTR(WebUserNameId)].as<String>();
    data.webPassword = jsonBuffer[FPSTR(WebPasswordId)].as<String>();
    data.homeKitPairData = jsonBuffer[FPSTR(HomeKitPairDataId)].as<String>();
    data.sensorsRefreshInterval = jsonBuffer[FPSTR(SensorsRefreshIntervalId)].as<uint64_t>();
    data.showDisplayInF = jsonBuffer[FPSTR(ShowDisplayInFId)].as<bool>();

    return true;
}

void config::reset()
{
    data.setDefaults();
    requestSave = true;
}

String config::readFromFile(const String &filePath)
{
    String result;

    File file = LittleFS.open(filePath, "r");
    if (!file)
    {
        return result;
    }

    while (file.available())
    {
        result += (char)file.read();
    }

    file.close();
    return result;
}

bool config::writeToFile(const String &fileName, const String &contents)
{
    File file = LittleFS.open(fileName, "w");
    if (!file)
    {
        return false;
    }

    const int bytesWritten = file.print(contents);
    if (bytesWritten == 0)
    {
        return false;
    }

    file.close();
    return true;
}

void config::save()
{
    Serial.println(F("Saving configuration"));
    DynamicJsonDocument jsonBuffer(1024);

    jsonBuffer[FPSTR(HostNameId)] = data.hostName;
    jsonBuffer[FPSTR(WebUserNameId)] = data.webUserName;
    jsonBuffer[FPSTR(WebPasswordId)] = data.webPassword;
    jsonBuffer[FPSTR(HomeKitPairDataId)] = data.homeKitPairData;
    jsonBuffer[FPSTR(SensorsRefreshIntervalId)] = data.sensorsRefreshInterval;
    jsonBuffer[FPSTR(ShowDisplayInFId)] = data.showDisplayInF;

    String json;
    serializeJson(jsonBuffer, json);

    Serial.println(json);

    if (writeToFile(ConfigFilePath, json))
    {
        const auto checksum = sha1(json);
        if (!writeToFile(ConfigChecksumFilePath, checksum))
        {
            Serial.println(F("Failed to write config checksum file"));
        }
    }
    else
    {
        Serial.println(F("Failed to write config file"));
    }

    callChangeListeners();
}

void config::loop()
{
    if (requestSave)
    {
        requestSave = false;
        save();
    }
}

String config::getAllConfigAsJson()
{
    loop();
    return readFromFile(ConfigFilePath);
}
