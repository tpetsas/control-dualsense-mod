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
 */

Config::Config (const char *iniPath) {
    char value[255] = "";

    GetPrivateProfileStringA("app", "debug", "false", value, sizeof(value), iniPath);
    isDebugMode = strcmp(value, "true") == 0;

    if (GetLastError() == ERROR_FILE_NOT_FOUND) {
        _LOG("%s is not an INI file; using config defaults...", iniPath);
        isDebugMode = false;
        return;
    }

    memset(value, 0, sizeof(value));
}

void Config::print() {
    _LOG("Config: [debug mode: %s]",
        isDebugMode ? "true" : "false"
    );
}
