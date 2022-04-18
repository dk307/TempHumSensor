#pragma once

#include <Print.h>

#define LOG_FORMAT 3

// Streaming operator for serial print use.

template <class T>
inline Print &operator<<(Print &obj, T &&arg)
{
    obj.print(std::forward<T>(arg));
    return obj;
}

static const char endl[] PROGMEM = "\r\n";
static const char errorPrefix[] PROGMEM = "ERROR [";
static const char warnPrefix[] PROGMEM = "WARN  [";
static const char infoPrefix[] PROGMEM = "INFO  [";
static const char debugPrefix[] PROGMEM = "DEBUG [";
static const char tracePrefix[] PROGMEM = "TRACE [";
static const char logSeperator1[] PROGMEM = "[";
static const char logSeperator2[] PROGMEM = "]";
static const char logSeperator3[] PROGMEM = "] : ";

#if LOG_FORMAT >= 0
#define LOG_ERROR(content)                                                                                   \
    do                                                                                                       \
    {                                                                                                        \
        Serial << FPSTR(logSeperator1) << millis() << FPSTR(logSeperator2) << FPSTR(errorPrefix) << __FILE__ \
               << FPSTR(logSeperator3) << content << FPSTR(endl);                                            \
    } while (0)
#else
#define LOG_ERROR(content) \
    do                     \
    {                      \
    } while (0)
#endif

#if LOG_FORMAT >= 1
#define LOG_WARNING(content)                                                                                \
    do                                                                                                      \
    {                                                                                                       \
        Serial << FPSTR(logSeperator1) << millis() << FPSTR(logSeperator2) << FPSTR(warnPrefix) << __FILE__ \
               << FPSTR(logSeperator3) << content << endl;                                                  \
    } while (0)
#else
#define LOG_WARNING(svc, content) \
    do                            \
    {                             \
    } while (0)
#endif

#if LOG_FORMAT >= 2
#define LOG_INFO(content)                                                                                   \
    do                                                                                                      \
    {                                                                                                       \
        Serial << FPSTR(logSeperator1) << millis() << FPSTR(logSeperator2) << FPSTR(infoPrefix) << __FILE__ \
               << FPSTR(logSeperator3) << content << endl;                                                  \
    } while (0)
#else
#define LOG_INFO(content) \
    do                    \
    {                     \
    } while (0)
#endif

#if LOG_FORMAT >= 3
#define LOG_DEBUG(content)                                                                                   \
    do                                                                                                       \
    {                                                                                                        \
        Serial << FPSTR(logSeperator1) << millis() << FPSTR(logSeperator2) << FPSTR(debugPrefix) << __FILE__ \
               << FPSTR(logSeperator3) << content << endl;                                                   \
    } while (0)
#else
#define LOG_DEBUG(content) \
    do                     \
    {                      \
    } while (0)
#endif

#if LOG_FORMAT >= 4
#define LOG_TRACE(content)                                                                                   \
    do                                                                                                       \
    {                                                                                                        \
        Serial << FPSTR(logSeperator1) << millis() << FPSTR(logSeperator2) << FPSTR(tracePrefix) << __FILE__ \
               << FPSTR(logSeperator3) << content << endl;                                                   \
    } while (0)
#else
#define LOG_TRACE(content) \
    do                     \
    {                      \
    } while (0)
#endif
