#pragma once

#include <Adafruit_Sensor.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "changeCallback.h"

class hardware
{
public:
    void begin();
    void loop();

    float getTemperatureC() const
    {
        return temperature;
    }

    float getHumidity() const
    {
        return humidity;
    }

    void showExternalMessages(const String& line1, const String& line2);

    static hardware instance;

    changeCallBack temperatureChangeCallback;
    changeCallBack humidityChangeCallback;

private:
    // DHT
    float temperature{NAN};
    float humidity{NAN};

    // SHT31
    const int SHT31Address = 0x44;
    Adafruit_SHT31  tempHumSensor;
    uint64_t lastRead{0};

    // SSD1306
    const int ScreenAddress = 0x3C;
    const int SDAWire = 4;
    const int SCLWire = 5;
    Adafruit_SSD1306 display{128, 64, &Wire, -1};

    bool refreshDisplay{false};
    bool updateTempNow{true};

    String externalLine1;
    String externalLine2;

    bool dhtUpdate();
    void updateDisplay();
    void display2Lines(const String &first, const String &second);
    static float roundPlaces(float val, int places);
    void invertPixels();
};