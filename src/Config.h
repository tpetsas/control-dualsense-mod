/*
 * Copyright (C) 2024 Thanasis Petsas <thanpetsas@gmail.com>
 * Licence: MIT Licence
 */

#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>

class Config
{
public:
    bool isDebugMode = false;
    std::unordered_map<std::string, std::vector<uint8_t>> weaponMap;
    Config() : isDebugMode(false) {};
    Config(const char *iniPath);
    void print();
};
