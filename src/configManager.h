#pragma once
#include "changeCallBack.h"

#include <ArduinoJson.h>

class DataStorage
{
public:
    void read(uint32_t srcAddress, uint32_t *desAddress, uint32_t size);
    void write(uint32_t desAddress, uint32_t *srcAddress, uint32_t size);
    void save();
};

struct configData
{
    String hostName;
    String webUserName;
    String webPassword;
    std::vector<uint8_t> homeKitPairData;
    uint64_t sensorsRefreshInterval;
    bool showDisplayInF;

    configData()
    {
        setDefaults();
    }

    void setDefaults()
    {
        const auto defaultUserIDPassword = F("admin");
        hostName.clear();
        webUserName = defaultUserIDPassword;
        webPassword = defaultUserIDPassword;
        homeKitPairData.resize(0);
        sensorsRefreshInterval = 5 * 1000;
        showDisplayInF = false;
    }
};

class config : public changeCallBack
{
public:
    configData data;
    bool begin();
    void save();
    void reset();
    void loop();

    static void erase();
    static config instance;

    String getAllConfigAsJson();

    // does not restore to memory, needs reboot
    bool restoreAllConfigAsJson(const std::vector<uint8_t> &json, const String &md5);

private:
    config() {}
    static String readFile(const String &fileName);

    template <class... T>
    static String md5Hash(T&&... data);

    template <class... T>
    static size_t writeToFile(const String &fileName, T&&... contents);

    template<class T>
    bool deserializeToJson(const T &data, DynamicJsonDocument &jsonDocument);

    bool requestSave{false};
};
