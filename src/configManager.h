#pragma once
#include "config.h"

#include "changeCallBack.h"

struct configData
{
    String hostName;
    String webUserName;
    String webPassword;
    String homeKitPairData;
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
        homeKitPairData = String();
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
