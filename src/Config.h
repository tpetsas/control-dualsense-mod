/*
 * Copyright (C) 2024 Thanasis Petsas <thanpetsas@gmail.com>
 * Licence: MIT Licence
 */

#pragma once

class Config
{
public:
    bool isDSXVersion2 = false;
    bool isDebugMode = false;
    Config() : isDSXVersion2(false), isDebugMode(false) {};
    Config(const char *iniPath);
    void print();
};
