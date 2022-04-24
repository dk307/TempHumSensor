#pragma once

#include <ESPAsyncWebServer.h>

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
    static void firmwareUpdateComplete(AsyncWebServerRequest *request);
    
    static void handleEarlyDisconnect();

    // ajax
    static void sensorGet(AsyncWebServerRequest *request);
    static void wifiGet(AsyncWebServerRequest *request);
    static void informationGet(AsyncWebServerRequest *request);
    static void homekitGet(AsyncWebServerRequest *request);
    static void configGet(AsyncWebServerRequest *request);

    // helpers
    static bool isAuthenticated(AsyncWebServerRequest *request);
    static bool manageSecurity(AsyncWebServerRequest *request);
    static String getContentType(const String &filename);
    static void handleNotFound(AsyncWebServerRequest *request);
    static void handleFileRead(AsyncWebServerRequest *request);
    static bool isCaptivePortalRequest(AsyncWebServerRequest *request);
    static void redirectToRoot(AsyncWebServerRequest *request);
    static void handleError(AsyncWebServerRequest *request, const String& error, int code);

    static bool isIp(const String &str);
    static String toStringIp(const IPAddress &ip);

    AsyncWebServer httpServer{80};
};
