#include <Arduino.h>

#include "homeKit2.h"
#include "configManager.h"



extern "C"
{
#include "homeintegration.h"
}
#include "homekitintegrationcpp.h"

HomeKit2 HomeKit2::instance;

void HomeKit2::begin()
{
    set_callback_storage_change(storageChanged);

    /// We will use for this example only one accessory (possible to use a several on the same esp)
    // Our accessory type is light bulb , apple interface will proper show that
    hap_setbase_accessorytype(homekit_accessory_category_lightbulb);
    /// init base properties
    hap_initbase_accessory_service(config::instance.data.hostName.c_str(), "Yurik72", "0", "EspHapLed", "1.0");
}

void HomeKit2::storageChanged(char *szstorage, int bufsize)
{
}