#pragma once

#include <vector>
#include <functional>

class changeCallBack
{
public:
    void addConfigSaveCallback(std::function<void()> func)
    {
        configsavecallback.push_back(func);
    }

protected:
    void callChangeListeners()
    {
        for (auto &&ftn : configsavecallback)
        {
            ftn();
        }
    }

private:
    std::vector<std::function<void()>> configsavecallback;
};