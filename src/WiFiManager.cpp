#include <ESP8266WiFi.h>

#include "WiFiManager.h"
#include "configManager.h"

WifiManager WifiManager::instance;

void WifiManager::begin()
{
    rfcName = config::instance.data.hostName;
    rfcName.trim();

    if (rfcName.isEmpty())
    {
        rfcName = F("ESP-") + String(ESP.getChipId(), HEX);
    }

    rfcName = getRFC952Hostname(rfcName);

    Serial.println(F("RFC name is ") + rfcName);

    WiFi.mode(WIFI_STA);
    WiFi.persistent(true);

    if (!WiFi.SSID().isEmpty())
    {
        // trying to fix connection in progress hanging
        ETS_UART_INTR_DISABLE();
        wifi_station_disconnect();
        ETS_UART_INTR_ENABLE();
        WiFi.begin();
    }

    if (waitForConnectResult(timeout) == WL_CONNECTED)
    {
        // connected
        Serial.println(F("Connected to stored WiFi details"));
        Serial.println(WiFi.localIP());
    }
    else
    {
        // captive portal
        startCaptivePortal();
    }
}

// Upgraded default waitForConnectResult function to incorporate WL_NO_SSID_AVAIL, fixes issue #122
int8_t WifiManager::waitForConnectResult(unsigned long timeoutLength)
{
    // 1 and 3 have STA enabled
    if ((wifi_get_opmode() & 1) == 0)
    {
        return WL_DISCONNECTED;
    }
    using esp8266::polledTimeout::oneShot;
    oneShot timeout(timeoutLength); // number of milliseconds to wait before returning timeout error
    while (!timeout)
    {
        yield();
        if (WiFi.status() != WL_DISCONNECTED && WiFi.status() != WL_NO_SSID_AVAIL)
        {
            return WiFi.status();
        }
    }
    return -1; // -1 indicates timeout
}

void WifiManager::disconnect()
{
    WiFi.disconnect();
}

// function to forget current WiFi details and start a captive portal
void WifiManager::forget()
{
    disconnect();
    startCaptivePortal();

    Serial.println(PSTR("Requested to forget WiFi. Started Captive portal."));
}

// function to request a connection to new WiFi credentials
void WifiManager::setNewWifi(const String &newSSID, const String &newPass)
{
    ssid = newSSID;
    pass = newPass;
    reconnect = true;
}

// function to connect to new WiFi credentials
void WifiManager::connectNewWifi(const String &newSSID, const String &newPass)
{
    WiFi.setHostname(rfcName.c_str());

    // fix for auto connect racing issue
    if (!(WiFi.status() == WL_CONNECTED && (WiFi.SSID() == newSSID)))
    {
        // trying to fix connection in progress hanging
        ETS_UART_INTR_DISABLE();
        wifi_station_disconnect();
        ETS_UART_INTR_ENABLE();

        // store old data in case new network is wrong
        const String oldSSID = WiFi.SSID();
        const String oldPSK = WiFi.psk();

        WiFi.begin(newSSID.c_str(), newPass.c_str(), 0, NULL, true);
        delay(2000);

        if (WiFi.waitForConnectResult(timeout) != WL_CONNECTED)
        {
            Serial.println(PSTR("New connection unsuccessful"));
            if (!inCaptivePortal)
            {
                WiFi.begin(oldSSID, oldPSK, 0, NULL, true);
                if (WiFi.waitForConnectResult(timeout) != WL_CONNECTED)
                {
                    Serial.println(PSTR("Reconnection failed too"));
                    startCaptivePortal();
                }
                else
                {
                    Serial.println(PSTR("Reconnection successful"));
                    Serial.println(WiFi.localIP());
                }
            }
        }
        else
        {
            if (inCaptivePortal)
            {
                stopCaptivePortal();
            }

            Serial.println(PSTR("New connection successful"));
            Serial.println(WiFi.localIP());
        }
    }
}

// function to start the captive portal
void WifiManager::startCaptivePortal()
{
    Serial.println(F("Opened a captive portal"));

    WiFi.persistent(false);
    // disconnect sta, start ap
    WiFi.disconnect(); //  this alone is not enough to stop the autoconnecter
    WiFi.mode(WIFI_AP);
    WiFi.persistent(true);

    WiFi.softAP(rfcName);

    dnsServer = new DNSServer();

    /* Setup the DNS server redirecting all the domains to the apIP */
    dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer->start(53, F("*"), WiFi.softAPIP());

    Serial.println(F("192.168.4.1"));
    inCaptivePortal = true;
}

// function to stop the captive portal
void WifiManager::stopCaptivePortal()
{
    WiFi.mode(WIFI_STA);
    delete dnsServer;

    inCaptivePortal = false;
    callChangeListeners();
}

// return captive portal state
bool WifiManager::isCaptivePortal()
{
    return inCaptivePortal;
}

// return current SSID
IPAddress WifiManager::LocalIP()
{
    return WiFi.localIP();
}

String WifiManager::SSID()
{
    return WiFi.SSID();
}

int8_t WifiManager::RSSI()
{
    return WiFi.RSSI();
}

int8_t WifiManager::RssiAsQuality()
{
    const int8_t rssi = WiFi.RSSI();
    int quality = 0;

    if (rssi <= -100)
    {
        quality = 0;
    }
    else if (rssi >= -50)
    {
        quality = 100;
    }
    else
    {
        quality = 2 * (rssi + 100);
    }
    return quality;
}

// captive portal loop
void WifiManager::loop()
{
    if (inCaptivePortal)
    {
        // captive portal loop
        dnsServer->processNextRequest();
    }

    if (reconnect)
    {

        connectNewWifi(ssid, pass);
        reconnect = false;
    }
}

String WifiManager::getRFC952Hostname(const String &name)
{
    const int MaxLength = 24;
    String rfc952Hostname;
    rfc952Hostname.reserve(MaxLength);

    for (auto &&c : name)
    {
        if (isalnum(c) || c == '-')
        {
            rfc952Hostname += c;
        }
        if (rfc952Hostname.length() >= MaxLength)
        {
            break;
        }
    }

    // remove last -
    size_t i = rfc952Hostname.length() - 1;
    while (rfc952Hostname[i] == '-' && i > 0)
    {
        i--;
    }

    return rfc952Hostname.substring(0, i);
}
