#pragma once

#include <Print.h>
#include <functional>

#ifndef LOG_FORMAT
#define LOG_FORMAT 2
#endif

class CustomLogger : public Print
{
public:
    CustomLogger();
    typedef std::function<bool(const String &value)> RecvMsgHandler;

    using Print::write;
    size_t write(uint8_t value) override;

    void setMsgCallback(RecvMsgHandler callback)
    {
        this->callback = callback;
    }

    bool hasListener() const { return loggingEnabled; }
    void enableLogging() { loggingEnabled = true; }

private:
    RecvMsgHandler callback;
    String current;
    bool loggingEnabled{true};
};

extern CustomLogger Logger;

template <class T>
inline Print &operator<<(Print &obj, T &&arg)
{
    obj.print(std::forward<T>(arg));
    return obj;
}

static const char endl[] PROGMEM = "\r\n";

static const char logSeperator1[] PROGMEM = "[";
static const char logSeperator2[] PROGMEM = "]";
static const char logSeperator3[] PROGMEM = "] : ";

#if LOG_FORMAT >= 0
static const char errorPrefix[] PROGMEM = " ERROR [";
#define LOG_ERROR(content)                                                                                   \
    if (Logger.hasListener())                                                                                \
    {                                                                                                        \
        Logger << FPSTR(logSeperator1) << millis() << FPSTR(logSeperator2) << FPSTR(errorPrefix) << __FILE__ \
               << FPSTR(logSeperator3) << content << FPSTR(endl);                                            \
    }
#else
#define LOG_ERROR(content) \
    do                     \
    {                      \
    } while (0)
#endif

#if LOG_FORMAT >= 1
static const char warnPrefix[] PROGMEM = " WARN  [";
#define LOG_WARNING(content)                                                                                \
    if (Logger.hasListener())                                                                               \
    {                                                                                                       \
        Logger << FPSTR(logSeperator1) << millis() << FPSTR(logSeperator2) << FPSTR(warnPrefix) << __FILE__ \
               << FPSTR(logSeperator3) << content << endl;                                                  \
    }
#else
#define LOG_WARNING(svc, content) \
    do                            \
    {                             \
    } while (0)
#endif

#if LOG_FORMAT >= 2
static const char infoPrefix[] PROGMEM = " INFO  [";
#define LOG_INFO(content)                                                                                   \
    if (Logger.hasListener())                                                                               \
    {                                                                                                       \
        Logger << FPSTR(logSeperator1) << millis() << FPSTR(logSeperator2) << FPSTR(infoPrefix) << __FILE__ \
               << FPSTR(logSeperator3) << content << endl;                                                  \
    }
#else
#define LOG_INFO(content) \
    do                    \
    {                     \
    } while (0)
#endif

#if LOG_FORMAT >= 3
static const char debugPrefix[] PROGMEM = " DEBUG [";
#define LOG_DEBUG(content)                                                                                   \
    if (Logger.hasListener())                                                                                \
    {                                                                                                        \
        Logger << FPSTR(logSeperator1) << millis() << FPSTR(logSeperator2) << FPSTR(debugPrefix) << __FILE__ \
               << FPSTR(logSeperator3) << content << endl;                                                   \
    }
#else
#define LOG_DEBUG(content) \
    do                     \
    {                      \
    } while (0)
#endif

#if LOG_FORMAT >= 4
static const char tracePrefix[] PROGMEM = " TRACE [";
#define LOG_TRACE(content)                                                                                   \
    if (Logger.hasListener())                                                                                \
    {                                                                                                        \
        Logger << FPSTR(logSeperator1) << millis() << FPSTR(logSeperator2) << FPSTR(tracePrefix) << __FILE__ \
               << FPSTR(logSeperator3) << content << endl;                                                   \
    }
#else
#define LOG_TRACE(content) \
    do                     \
    {                      \
    } while (0)
#endif
