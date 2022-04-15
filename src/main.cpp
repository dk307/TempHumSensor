#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <coredecls.h>

#include "webServer.h"
#include "configManager.h"
#include "WiFiManager.h"
#include "operations.h"
#include "hardware.h"
#include "homeKit2.h"
#include "logging.h"

void setup(void)
{
	Serial.begin(115200);
 
	LittleFS.begin();
	operations::instance.begin();
	config::instance.begin();
	WifiManager::instance.begin();
	WebServer::instance.begin();
	hardware::instance.begin();
	homeKit2::instance.begin();
	LOG_INFO(F("Setup finished"));
}

void loop(void)
{
	WifiManager::instance.loop();
	config::instance.loop();
	hardware::instance.loop();
	operations::instance.loop();
	homeKit2::instance.loop();
}
