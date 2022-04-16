#pragma once
#include "changeCallBack.h"

class DataStorage
{
public:
    void read(uint32_t srcAddress, uint32_t *desAddress,uint32_t size);
    void write(uint32_t desAddress, uint32_t *srcAddress, uint32_t size);
    void save();
};

struct configData
{
    String hostName;
    String webUserName;
    String webPassword;
    std::vector<byte> homeKitPairData;
    uint64_t sensorsRefreshInterval;
    bool showDisplayInF;

    configData()
    {
        setDefaults();
    }

    void setDefaults()
    {
        hostName = String();
        webUserName = F("admin");
        webPassword = F("admin");
        homeKitPairData.empty();
        sensorsRefreshInterval = 15 * 1000;
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

private:
    config() {}
    static String readFromFile(const String &filePath);
    static bool writeToFile(const String &fileName, const String &contents);
    bool requestSave = false;
};