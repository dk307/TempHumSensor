#pragma once

#include <homekit/homekit.h>
#include <homekit/characteristics.h>

class homeKit2
{
public:
    void begin();
    void loop();

    static homeKit2 instance;

private:
    homeKit2() {};
    static void storageChanged(char *szstorage, int bufsize);
};

extern homekit_server_config_t config;