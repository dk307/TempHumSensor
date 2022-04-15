#pragma once

class HomeKit2
{
public:
    void begin();
    void loop();

    static HomeKit2 instance;

private:
    static void storageChanged(char *szstorage, int bufsize);
};