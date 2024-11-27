#pragma once
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

#define _LOG(...) g_logger.Log(__VA_ARGS__)
