/*
 * Copyright (C) 2024 Thanasis Petsas <thanpetsas@gmail.com>
 * Licence: MIT Licence
 */

#include "Config.h"
#include "Logger.h"
#include <windows.h>

/*
 * sample INI content:
 *
 * [app]
 * debug=true
 *
 * [dsx]
 * version=2
 *
 */

Config::Config (const char *iniPath) {
    char value[255] = "";

    GetPrivateProfileStringA("app", "debug", "false", value, sizeof(value), iniPath);
    isDebugMode = strcmp(value, "true") == 0;

    if (GetLastError() == ERROR_FILE_NOT_FOUND) {
        _LOG("%s is not an INI file; using config defaults...", iniPath);
        isDebugMode = false;
        isDSXVersion2 = false;
        return;
    }

    memset(value, 0, sizeof(value));
    GetPrivateProfileStringA("dsx", "version", "3", value, sizeof(value), iniPath);
    isDSXVersion2 = strncmp(value, "2", 1) == 0;
}

void Config::print() {
    _LOG("Config: [debug mode: %s] [DSX legacy version: %s]",
        isDebugMode ? "true" : "false",
        isDSXVersion2 ? "true" : "false"
    );
}
