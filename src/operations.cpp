#include "operations.h"

#include <Arduino.h>

#define USE_LITTLEFS false
#define ESP_MRD_USE_LITTLEFS false
#define ESP_MRD_USE_SPIFFS false
#define ESP_MRD_USE_EEPROM true

#define MRD_TIMES 5
#define MRD_TIMEOUT 10
#define MRD_ADDRESS 0
#define MULTIRESETDETECTOR_DEBUG false
#include <ESP_MultiResetDetector.h>

#include "WiFiManager.h"
#include "configManager.h"
#include "logging.h"

operations operations::instance;

void operations::factoryReset()
{
	LOG_INFO("Doing Factory Reset");
	system_restore();
	config::erase();
	ESP.reset();
}

void operations::begin()
{
	mrd = new MultiResetDetector(MRD_TIMEOUT, MRD_ADDRESS);

	if (mrd->detectMultiReset())
	{
		LOG_WARNING(F("Detected Multi Reset Event!!!!"));
		// factoryReset();
	}
	else
	{
		LOG_INFO(F("Not detected Multi Reset Event"));
	}
}

void operations::reboot()
{
	rebootPending = true;
}

void operations::loop()
{
	if (mrd)
	{
		mrd->loop();
		if (!mrd->waitingForMRD())
		{
			delete mrd;
			mrd = nullptr;
		}
	}

	if (rebootPending)
	{
		LOG_INFO(F("Restarting..."));
		rebootPending = false;
		ESP.reset();
	}
}