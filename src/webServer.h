#pragma once

#include <ESPAsyncWebServer.h>
#include <vector>

class WebServer
{
public:
    void begin();
    static WebServer instance;

private:
    WebServer() {}
    void serverRouting();

    // handler
    static void handleLogin(AsyncWebServerRequest *request);
    static void handleLogout(AsyncWebServerRequest *request);
    static void wifiUpdate(AsyncWebServerRequest *request);
    static void webLoginUpdate(AsyncWebServerRequest *request);
    static void otherSettingsUpdate(AsyncWebServerRequest *request);
    static void factoryReset(AsyncWebServerRequest *request);
    static void homekitReset(AsyncWebServerRequest *request);
    static void restartDevice(AsyncWebServerRequest *request);

    static void firmwareUpdateUpload(AsyncWebServerRequest *request,
                                     const String &filename,
                                     size_t index,
                                     uint8_t *data,
                                     size_t len,
                                     bool final);
    static void rebootOnUploadComplete(AsyncWebServerRequest *request);

    static void restoreConfigurationUpload(AsyncWebServerRequest *request,
                                           const String &filename,
                                           size_t index,
                                           uint8_t *data,
                                           size_t len,
                                           bool final);

    static void handleEarlyUpdateDisconnect();

    // ajax
    static void sensorGet(AsyncWebServerRequest *request);
    static void wifiGet(AsyncWebServerRequest *request);
    static void informationGet(AsyncWebServerRequest *request);
    static void homekitGet(AsyncWebServerRequest *request);
    static void configGet(AsyncWebServerRequest *request);

    // helpers
    static bool isAuthenticated(AsyncWebServerRequest *request);
    static bool manageSecurity(AsyncWebServerRequest *request);
    static void handleNotFound(AsyncWebServerRequest *request);
    static void handleFileRead(AsyncWebServerRequest *request);
    static bool isCaptivePortalRequest(AsyncWebServerRequest *request);
    static void redirectToRoot(AsyncWebServerRequest *request);
    static void handleError(AsyncWebServerRequest *request, const String &error, int code);
    void onEventConnect(AsyncEventSourceClient *client);
    void onLoggingConnect(AsyncEventSourceClient *client);
    bool filterEvents(AsyncWebServerRequest *request);

    static bool isIp(const String &str);
    static String toStringIp(const IPAddress &ip);
    static String GetUptime();
    template <class Array, class K, class T>
    static void addKeyValueObject(Array &array, const K &key, const T &value);
    template <class V, class T>
    static void addToJsonDoc(V &doc, T id, float value);
    void notifyTemperatureChange();
    void notifyHumidityChange();
    bool sendLogs(const String& data);

    AsyncWebServer httpServer{80};
    AsyncEventSource events{"/events"};
    AsyncEventSource logging{"/logs"};
    std::unique_ptr<std::vector<uint8_t>> restoreConfigData;
};
