#include "logging.h"
#include "logprintf.h"

#include <arduino.h>
#include <stdarg.h>
#include <StreamString.h>

CustomLogger::CustomLogger()
{
    current.reserve(64);
}

size_t CustomLogger::write(uint8_t value)
{
    if (callback && loggingEnabled)
    {
        if (value == '\n') {
            loggingEnabled = callback(current);
            current.clear();
        } else {
            current.concat(static_cast<const char>(value)); 
        }
    }
    return 1;
}

int loggerPrintf_P(const char *format, ...)
{
    if (Logger.hasListener())
    {
        va_list arg;
        va_start(arg, format);
        char temp[64];
        char *buffer = temp;
        size_t len = vsnprintf_P(temp, sizeof(temp), format, arg);
        va_end(arg);
        if (len > sizeof(temp) - 1)
        {
            buffer = new (std::nothrow) char[len + 1];
            if (!buffer)
            {
                return 0;
            }
            va_start(arg, format);
            vsnprintf_P(buffer, len + 1, format, arg);
            va_end(arg);
        }
        len = Logger.write((const uint8_t *)buffer, len);
        if (buffer != temp)
        {
            delete[] buffer;
        }
        return len;
    }
    return 0;
}

CustomLogger Logger;