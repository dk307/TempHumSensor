#include <Arduino.h>

#include <LittleFS.h>
#include <base64.h> // from esphap
#include <MD5Builder.h>
#include "logging.h"

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

template <class... T>
String config::md5Hash(T &&...data)
{
    MD5Builder hashBuilder;
    hashBuilder.begin();
    hashBuilder.add(data...);
    hashBuilder.calculate();
    return hashBuilder.toString();
}

template <class... T>
size_t config::writeToFile(const String &fileName, T &&...contents)
{
    File file = LittleFS.open(fileName, "w");
    if (!file)
    {
        return 0;
    }

    const auto bytesWritten = file.write(contents...);
    file.close();
    return bytesWritten;
}

void config::erase()
{
    LittleFS.remove(FPSTR(ConfigChecksumFilePath));
    LittleFS.remove(FPSTR(ConfigFilePath));
}

bool config::begin()
{
    const auto configData = readFile(FPSTR(ConfigFilePath));

    if (configData.isEmpty())
    {
        LOG_INFO(F("No stored config found"));
        reset();
        return false;
    }

    DynamicJsonDocument jsonDocument(2048);
    if (!deserializeToJson(configData.c_str(), jsonDocument))
    {
        reset();
        return false;
    }

    // read checksum from file
    const auto readChecksum = readFile(FPSTR(ConfigChecksumFilePath));
    const auto checksum = md5Hash(configData);

    if (!checksum.equalsIgnoreCase(readChecksum))
    {
        LOG_ERROR(F("Config data checksum mismatch"));
        reset();
        return false;
    }

    data.hostName = jsonDocument[FPSTR(HostNameId)].as<String>();
    data.webUserName = jsonDocument[FPSTR(WebUserNameId)].as<String>();
    data.webPassword = jsonDocument[FPSTR(WebPasswordId)].as<String>();
    data.sensorsRefreshInterval = jsonDocument[FPSTR(SensorsRefreshIntervalId)].as<uint64_t>();
    data.showDisplayInF = jsonDocument[FPSTR(ShowDisplayInFId)].as<bool>();

    const auto encodedHomeKitData = jsonDocument[FPSTR(HomeKitPairDataId)].as<String>();

    const auto size = base64_decoded_size(reinterpret_cast<const unsigned char *>(encodedHomeKitData.c_str()),
                                          encodedHomeKitData.length());
    data.homeKitPairData.resize(size);

    base64_decode_(reinterpret_cast<const unsigned char *>(encodedHomeKitData.c_str()),
                   encodedHomeKitData.length(), data.homeKitPairData.data());

    data.homeKitPairData.shrink_to_fit();

    LOG_DEBUG(F("Loaded Config from file"));

    return true;
}

void config::reset()
{
    data.setDefaults();
    requestSave = true;
}

void config::save()
{
    LOG_INFO(F("Saving configuration"));

    DynamicJsonDocument jsonDocument(2048);

    jsonDocument[FPSTR(HostNameId)] = data.hostName.c_str();
    jsonDocument[FPSTR(WebUserNameId)] = data.webUserName.c_str();
    jsonDocument[FPSTR(WebPasswordId)] = data.webPassword.c_str();

    const auto requiredSize = base64_encoded_size(data.homeKitPairData.data(), data.homeKitPairData.size());
    const auto encodedData = std::make_unique<unsigned char[]>(requiredSize + 1);
    base64_encode_(data.homeKitPairData.data(), data.homeKitPairData.size(), encodedData.get());

    jsonDocument[FPSTR(HomeKitPairDataId)] = encodedData.get();
    jsonDocument[FPSTR(SensorsRefreshIntervalId)] = data.sensorsRefreshInterval;
    jsonDocument[FPSTR(ShowDisplayInFId)] = data.showDisplayInF;

    String json;
    serializeJson(jsonDocument, json);

    if (writeToFile(FPSTR(ConfigFilePath), json.c_str(), json.length()) == json.length())
    {
        const auto checksum = md5Hash(json);
        if (writeToFile(FPSTR(ConfigChecksumFilePath), checksum.c_str(), checksum.length()) != checksum.length())
        {
            LOG_ERROR(F("Failed to write config checksum file"));
        }
    }
    else
    {
        LOG_ERROR(F("Failed to write config file"));
    }

    LOG_INFO(F("Saving Configuration done"));
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

String config::readFile(const String &fileName)
{
    File file = LittleFS.open(fileName, "r");
    if (!file)
    {
        return String();
    }

    const auto json = file.readString();
    file.close();
    return json;
}

String config::getAllConfigAsJson()
{
    loop(); // save if needed
    return readFile(FPSTR(ConfigFilePath));
}

bool config::restoreAllConfigAsJson(const std::vector<uint8_t> &json, const String &hashMd5)
{
    DynamicJsonDocument jsonDocument(2048);
    if (!deserializeToJson(json, jsonDocument))
    {
        return false;
    }

    const auto expectedMd5 = md5Hash(json.data(), json.size());
    if (!expectedMd5.equalsIgnoreCase(hashMd5))
    {
        LOG_ERROR(F("Uploaded Md5 for config does not match. File md5:") << expectedMd5);
        return false;
    }

    if (writeToFile(FPSTR(ConfigFilePath), json.data(), json.size()) != json.size())
    {
        return false;
    }

    if (writeToFile(FPSTR(ConfigChecksumFilePath), hashMd5.c_str(), hashMd5.length()) != hashMd5.length())
    {
        return false;
    }
    return true;
}

template <class T>
bool config::deserializeToJson(const T &data, DynamicJsonDocument &jsonDocument)
{
    DeserializationError error = deserializeJson(jsonDocument, data);

    // Test if parsing succeeds.
    if (error)
    {
        LOG_ERROR(F("deserializeJson for config failed: ") << error.f_str());
        return false;
    }
    return true;
}
