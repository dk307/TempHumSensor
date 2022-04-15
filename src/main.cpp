#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <coredecls.h>

#include "webServer.h"
#include "configManager.h"
#include "WiFiManager.h"
#include "operations.h"
#include "hardware.h"

void setup(void)
{
	Serial.begin(115200);
	LittleFS.begin();
	operations::instance.begin();
	config::instance.begin();
	WifiManager::instance.begin();
	WebServer::instance.begin();
	hardware::instance.begin();
}

void loop(void)
{
	WifiManager::instance.loop();
	config::instance.loop();
	hardware::instance.loop();
	operations::instance.loop();
}
