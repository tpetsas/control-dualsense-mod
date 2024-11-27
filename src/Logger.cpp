#include "Logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <share.h>

FILE* logfile = nullptr;

Logger::Logger()
{
}

Logger::~Logger()
{
}

bool Logger::Open(const char * path)
{
    logfile = _fsopen(path, "w", _SH_DENYWR);
    return logfile != NULL;
}

void Logger::Close()
{
    if (logfile) fclose(logfile);
}

void Logger::Log(const char * format, ...)
{
    static char outputBuf[8192];
    if (logfile) {
        va_list args; va_start(args, format);
        vsnprintf(outputBuf, sizeof(outputBuf), format, args);
        va_end(args);
        fprintf(logfile, "%s\n", outputBuf);
        fflush(logfile);
    }
}

