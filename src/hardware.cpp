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
        LOG_ERROR(F("Fail to start Temp/Hum Sensor"));
    }

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
    const bool changed = dhtUpdate();
    if (changed || refreshDisplay)
    {
        refreshDisplay = false;
        updateDisplay();
    }
}

bool hardware::dhtUpdate()
{
    bool changed = false;
    const auto now = millis();
    if ((now - lastRead > config::instance.data.sensorsRefreshInterval) || updateTempNow)
    {
        const auto temp = roundPlaces(tempHumSensor.readTemperature(), 1);
        const auto hum = roundPlaces(tempHumSensor.readHumidity(), 0);

        if (temperature != temp)
        {
            LOG_DEBUG(F("Temp: ") << temp);
            temperature = temp;
            changed = true;
            temperatureChangeCallback.callChangeListeners();
        }

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

    return changed;
}

void hardware::display2Lines(const String &first, const String &second)
{
    display.clearDisplay();
    // get text bounds to make text middle justify
    int16_t x1;
    int16_t y1;
    uint16_t w;
    uint16_t h;

    display.getTextBounds(first, 0, 23, &x1, &y1, &w, &h);
    display.setCursor((display.width() - w) / 2, 23);
    display.write(first.c_str());

    display.getTextBounds(second, 0, h + 10 + 23, &x1, &y1, &w, &h);
    display.setCursor((display.width() - w) / 2, h + 10 + 23);
    display.print(second.c_str());

    if (random(10) == 1) // 1 in 10 screen saver
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
        display2Lines(externalLine1, externalLine2);
    }
    else if (WifiManager::instance.isCaptivePortal())
    {
        display.setTextSize(1);
        display2Lines(F("AP Mode"), WifiManager::instance.getAPForCaptiveMode());
    }
    else
    {
        if (!isnan(temperature) && !isnan(humidity))
        {
            display.setTextSize(2);
            const auto displayTemperature = config::instance.data.showDisplayInF ? temperature * 9 / 5 + 32 : temperature;
            const auto displayTemperatureUnit = config::instance.data.showDisplayInF ? F(" F") : F(" C");
            display2Lines(String(displayTemperature, 1) + displayTemperatureUnit, String(humidity, 0) + F(" %"));
        }
        else
        {
            display.setTextSize(1);
            display2Lines(F("Listening at"), WifiManager::LocalIP().toString());
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
