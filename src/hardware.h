#pragma once

#define SSD1306_NO_SPLASH 
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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

    static hardware instance;

private:
    // DHT
    float temperature{NAN};
    float humidity{NAN};
    const uint8_t DhtPin = 13;
    const uint8_t DhtType = DHT22; // DHT 22 (AM2302)
    DHT dht{DhtPin, DhtType};
    uint64_t lastRead{0};

    // SSD1306
    const int ScreenAddress = 0x3C;
    const int SDAWire = 4;
    const int SCLWire = 5;
    Adafruit_SSD1306 display{128, 64, &Wire, -1};

    bool refreshDisplay{false};


    bool dhtUpdate();
    void updateDisplay();
    void display2Lines(const String& first, const String& second);
    static float round2Places(float val);
};