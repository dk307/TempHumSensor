#include "hardware.h"
#include "configManager.h"
#include "WiFiManager.h"
#include "logging.h"

#include <math.h>

#include "Fonts/font.h"

hardware hardware::instance;

void hardware::begin()
{
    const auto ftn = [this]
    {
        LOG_DEBUG(F("Display refresh needed"));
        refreshDisplay = true;
    };

    config::instance.addConfigSaveCallback(ftn);
    WifiManager::instance.addConfigSaveCallback(ftn);

    Wire.begin(SDAWire, SCLWire);

    if (!display.begin(SSD1306_SWITCHCAPVCC, ScreenAddress))
    {
        LOG_ERROR(F("SSD1306 allocation failed"));
    }

    if (!tempHumSensor.begin(SHT31Address))
    {
        LOG_ERROR(F("Failed to start SHT31 Sensor"));
    }

    tempHumSensor.heater(true);

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setTextWrap(false);
    display.setFont(&FreeSerif9pt7b);
    updateDisplay();
}

float hardware::roundPlaces(float val, int places)
{
    if (!isnan(val))
    {
        const auto expVal = pow(10, places);
        const auto result = float(uint64_t(expVal * val + 0.5)) / expVal;
        return result;
    }
    return val;
}

void hardware::loop()
{
    const bool changed = sensorUpdate();
    if (changed || refreshDisplay)
    {
        refreshDisplay = false;
        updateDisplay();
    }
}

bool hardware::sensorUpdate()
{
    bool changed = false;
    const auto now = millis();
    if ((now - lastRead > config::instance.data.sensorsRefreshInterval) || updateTempNow)
    {
        float hum = tempHumSensor.readHumidity();
        hum = roundPlaces(hum, 0);

        if (humidity != hum)
        {
            LOG_DEBUG(F("Hum: ") << hum);
            humidity = hum;
            changed = true;
            humidityChangeCallback.callChangeListeners();
        }
        lastRead = now;
        updateTempNow = false;
    }

    if (now - heaterTimeSwitch > 10000)
    {
        tempHumSensor.heater(!tempHumSensor.isHeaterEnabled());
        heaterTimeSwitch = now;
    }

    return changed;
}

void hardware::display2Lines(const String &first, const String &second, bool firstSmall, bool secondSmall)
{
    display.clearDisplay();

    const auto height = 64;
    const auto firstLineHeight = firstSmall ? height / 3 : height / 2;

    // get text bounds to make text middle justify
    int16_t x1;
    int16_t y1;
    uint16_t w;
    uint16_t h;

    if (!first.isEmpty())
    {
        display.setTextSize(firstSmall ? 1 : 2);
        display.getTextBounds(first, 0, firstLineHeight, &x1, &y1, &w, &h);
        display.setCursor((display.width() - w) / 2, firstLineHeight - (firstLineHeight - h) / 2);
        display.write(first.c_str());
    }

    if (!second.isEmpty())
    {
        const auto secondLineHeight = height - firstLineHeight;
        display.setTextSize(secondSmall ? 1 : 2);
        display.getTextBounds(second, 0, secondLineHeight, &x1, &y1, &w, &h);
        display.setCursor((display.width() - w) / 2, height - (secondLineHeight - h) / 2);
        display.write(second.c_str());
    }

    if (random(15) == 1) // 1 in 15 screen saver
    {
        invertPixels();
    }

    display.display();
}

void hardware::invertPixels()
{
    for (auto x = 0; x < display.width(); x++)
    {
        for (int y = 0; y < display.height(); y++)
        {
            display.drawPixel(x, y, SSD1306_INVERSE);
        }
    }
}

void hardware::updateDisplay()
{
    if (!externalLine1.isEmpty() || !externalLine2.isEmpty())
    {
        const bool small = !externalLine2.isEmpty();
        display2Lines(externalLine1, externalLine2, small, small);
    }
    else if (WifiManager::instance.isCaptivePortal())
    {
        display.setTextSize(1);
        display2Lines(F("AP Mode"), WifiManager::instance.getAPForCaptiveMode(), true, true);
    }
    else
    {
        if (!isnan(humidity))
        {
            display.setTextSize(2);
            display2Lines("Humidity", String(humidity, 0) + F(" %"), true, false);
        }
        else
        {
            display.setTextSize(1);
            display2Lines(F("Listening at"), WifiManager::LocalIP().toString(), true, true);
        }
    }

    display.display();
}

void hardware::showExternalMessages(const String &line1, const String &line2)
{
    externalLine1 = line1;
    externalLine2 = line2;
    refreshDisplay = true;
}
