#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#define CUSTOM_PRINTF(fmt, ...) loggerPrintf_P(PSTR(fmt), ##__VA_ARGS__);

int loggerPrintf_P(const char *str, ...);
#ifdef __cplusplus
}
#endif