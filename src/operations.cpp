#include "operations.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <StreamString.h>

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
	LOG_INFO(F("Doing Factory Reset"));
	ESP.eraseConfig();
	system_restore();
	config::erase();
	LittleFS.format();
	reset();
}

void operations::begin()
{
	mrd = new MultiResetDetector(MRD_TIMEOUT, MRD_ADDRESS);

	if (mrd->detectMultiReset())
	{
		LOG_WARNING(F("Detected Multi Reset Event!!!!"));
		factoryReset();
	}
	else
	{
		LOG_INFO(F("Not detected Multi Reset Event"));
	}
	beginFS();

	Update.runAsync(true);
}

bool operations::beginFS()
{
	LOG_TRACE(F("Mounting FS"));
	if (LittleFS.begin())
	{
		return true;
	}

	LOG_INFO(F("Failed to mount, formating FS"));
	LittleFS.format();
	if (!LittleFS.begin())
	{
		LOG_ERROR(F("DANGER: Failed to mount even after formatting!"));

		delay(10000); // Make sure the debug message doesn't just float by.
		return false;
	}
	return true;
}

void operations::reboot()
{
	rebootPending = true;
}

bool operations::startUpdate(size_t length, const String &md5, String &error)
{
	LOG_INFO(F("Update call start with length:") << length);
	LOG_INFO(F("Current Sketch size:") << ESP.getSketchSize());
	LOG_INFO(F("Free sketch space:") << ESP.getFreeSketchSpace());

	const uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
	if (!Update.setMD5(md5.c_str()))
	{
		LOG_ERROR(F("Md5 Invalid: ") << error);
		return false;
	}

	if (Update.begin(maxSketchSpace))
	{
		LOG_DEBUG(F("Update begin successfull"));
		return true;
	}
	else
	{
		getUpdateError(error);
		LOG_ERROR(F("Update begin failed with ") << error);
		return false;
	}
}

bool operations::writeUpdate(const uint8_t *data, size_t length, String &error)
{
	LOG_DEBUG(F("Update write with length:") << length);
	LOG_DEBUG(F("Update stats Size:") << Update.size()
									  << F(" progress:") << Update.progress()
									  << F(" remaining:") << Update.remaining());
	const auto written = Update.write(const_cast<uint8_t *>(data), length);
	if (written == length)
	{
		LOG_DEBUG(F("Update write successful"));
		return true;
	}
	else
	{
		getUpdateError(error);
		LOG_ERROR(F("Update write failed with ") << error);
		return false;
	}
}

bool operations::endUpdate(String &error)
{
	LOG_DEBUG(F("Update end called"));

	if (Update.end(true))
	{
		LOG_INFO(F("Update end successful"));
		return true;
	}
	else
	{
		getUpdateError(error);
		LOG_ERROR(F("Update end failed with ") << error);
		return false;
	}
}

void operations::abortUpdate()
{
	LOG_DEBUG(F("Update end called"));
	if (Update.isRunning())
	{
		if (Update.end(true))
		{
			LOG_INFO(F("Aborted update"));
		}
		else
		{
			LOG_ERROR(F("Aborted update failed"));
		}
	}
}

bool operations::isUpdateInProgress()
{
	return Update.isRunning();
}

void operations::getUpdateError(String &error)
{
	StreamString streamString;
	Update.printError(streamString);
	error = std::move(streamString);
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
		rebootPending = false;
		reset();
	}
}

[[noreturn]] void operations::reset()
{
	LOG_INFO(F("Restarting..."));
	delay(2000); // for http response, etc to finish
	ESP.restart();
	for (;;)
	{
		delay(100);
	}
}