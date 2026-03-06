/*
 * Copyright (C) 2024 Thanasis Petsas <thanpetsas@gmail.com>
 * Licence: MIT Licence
 */

#include "Config.h"
#include "Logger.h"
#include <windows.h>
#include <unordered_map>
#include <sstream>

/*
 * sample INI content:
 *
 * [app]
 * debug=true
 * [weapons]
 * PISTOL_L2=0,0
 * PISTOL_R2=33,100,160,255
 *
 */

Config::Config (const char *iniPath) {
    char value[255] = "";
    char weapons[4096] = "";

    GetPrivateProfileStringA(
        "weapons", NULL, "", weapons,
        sizeof(weapons), iniPath
    );

    for (char* wep = weapons; *wep; wep += strlen(wep) + 1)
    {
        char wepBuffer[256];
        GetPrivateProfileStringA("weapons", wep, "", wepBuffer, sizeof(wepBuffer), iniPath);

        std::stringstream ss(wepBuffer);
        std::string item;
        while (std::getline(ss, item, ','))
        {
            uint8_t val = static_cast<uint8_t>(std::stoi(item));
            weaponMap[wep].push_back(val);
        }

        for (int i = weaponMap[wep].size(); i < 8; i++) {
            weaponMap[wep].push_back(0);
        }

    }

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
