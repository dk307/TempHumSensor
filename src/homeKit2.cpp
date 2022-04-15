#include <Arduino.h>

#include "homeKit2.h"
#include "configManager.h"

#include <arduino_homekit_server.h>

homeKit2 homeKit2::instance;

void homeKit2::begin()
{
    arduino_homekit_setup(&config);
}

void homeKit2::loop()
{
    arduino_homekit_loop();
}

void homeKit2::storageChanged(char *szstorage, int bufsize)
{
}