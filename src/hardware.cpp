#include "hardware.h"
#include "configManager.h"
#include "WiFiManager.h"

#include "Fonts/font.h"

hardware hardware::instance;

void hardware::begin()
{
    const auto ftn = [this]
    {
        Serial.println(F("Display refresh needed"));
        refreshDisplay = true;
    };
    config::instance.addConfigSaveCallback(ftn);
    WifiManager::instance.addConfigSaveCallback(ftn);

    dht.begin();

    Wire.begin(SDAWire, SCLWire);

    if (!display.begin(SSD1306_SWITCHCAPVCC, ScreenAddress))
    {
        Serial.println(F("SSD1306 allocation failed"));
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setTextWrap(false);
    display.setFont(&FreeSerif9pt7b);
    updateDisplay();
}

float hardware::round2Places(float val)
{
    if (!isnan(val))
    {
        return float(uint64_t(val * 100.0 + 0.5)) / 100.0;
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
    if (now - lastRead > config::instance.data.sensorsRefreshInterval)
    {
        const auto temp = round2Places(dht.readTemperature());
        const auto hum = round2Places(dht.readHumidity());

        if (temperature != temp)
        {
            Serial.print(F("Temp: "));
            Serial.println(temp, 4);
            temperature = temp;
            changed = true;
        }

        if (humidity != hum)
        {
            Serial.print(F("Hum: "));
            Serial.println(hum, 4);
            humidity = hum;
            changed = true;
        }
        lastRead = now;
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

    display.display();
}

void hardware::updateDisplay()
{
    if (WifiManager::instance.isCaptivePortal())
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
            display2Lines(String(displayTemperature, 2) + displayTemperatureUnit, String(humidity, 2) + F(" %"));
        }
        else
        {
            display.setTextSize(1);
            display2Lines(F("Listening at"), WifiManager::LocalIP().toString());
        }
    }

    display.display();
}
