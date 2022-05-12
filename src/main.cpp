#include <ESP8266WiFi.h>

#include "webServer.h"
#include "configManager.h"
#include "WiFiManager.h"
#include "operations.h"
#include "hardware.h"
#include "homeKit2.h"
#include "logging.h"

void setup(void)
{
	//Serial.begin(115200);

	operations::instance.begin();
	config::instance.begin();
	WifiManager::instance.begin();
	WebServer::instance.begin();
	hardware::instance.begin();
	homeKit2::instance.begin();
	LOG_INFO(F("Finish setup. Free heap: ") << ESP.getFreeHeap() / 1024 << F(" KB"));
}

void loop(void)
{
	WifiManager::instance.loop();
	config::instance.loop();
	homeKit2::instance.loop();
	hardware::instance.loop();
	operations::instance.loop();
}
