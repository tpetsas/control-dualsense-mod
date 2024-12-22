#pragma once

#include "Config.h"

class Logger
{
public:
    Logger();
    ~Logger();

    static bool Open(const char* path);
    static void Close();

    static void Log(const char* format, ...);
};

extern Logger g_logger;
extern Config g_config;

#define _LOG(...) g_logger.Log(__VA_ARGS__)

#define _LOGD(...) \
    do { \
        if (g_config.isDebugMode) { \
            _LOG(__VA_ARGS__); \
        } \
    } while (0)
