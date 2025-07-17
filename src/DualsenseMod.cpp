/*
 * Copyright (C) 2024 Thanasis Petsas <thanpetsas@gmail.com>
 * Licence: MIT Licence
 */

#include "DualsenseMod.h"

#include "Logger.h"
#include "Config.h"
#include "Utils.h"
#include "rva/RVA.h"
#include "minhook/include/MinHook.h"

// headers needed for dualsensitive
#include <udp.h>
#include <dualsensitive.h>
#include <IO.h>
#include <Device.h>
#include <Helpers.h>
#include <iostream>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <cinttypes>
#include <cstdlib>
#include <memory>
#include <algorithm>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include <mutex>
#include <map>

#define INI_LOCATION "./plugins/dualsense-mod.ini"

// TODO: move the following to a server utils file

PROCESS_INFORMATION serverProcInfo;


#include <shellapi.h>

bool launchServerElevated(const std::wstring& exePath = L"./plugins/dualsensitive-service.exe") {
    wchar_t fullExePath[MAX_PATH];
    if (!GetFullPathNameW(exePath.c_str(), MAX_PATH, fullExePath, nullptr)) {
        _LOG("Failed to resolve full path");
        return false;
    }

    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"runas";
    sei.lpFile = fullExePath;
    sei.nShow = SW_HIDE;
    sei.fMask = SEE_MASK_NO_CONSOLE;

    if (!ShellExecuteExW(&sei)) {
        DWORD err = GetLastError();
        _LOG("ShellExecuteEx failed: %lu", err);
        return false;
    }

    return true;
}
bool scheduledTaskExists(std::string taskName) {
    std::string query = "schtasks /query /TN \"" + taskName + "\" >nul 2>&1";
    int result = WinExec(query.c_str(), SW_HIDE);
    _LOG("Task exists: %d", result);
    return (result > 31);
}

bool launchServerTask() {
    std::string command = "schtasks /run /TN \"DualSensitive Service\" /I";
    int result = WinExec(command.c_str(), SW_HIDE);
    return (result > 31);
}

// schtasks /Create /TN "DualSensitive Service" /TR "wscript.exe \"C:\Program Files (x86)\Steam\steamapps\common\Control\plugins\launch-service.vbs\" \"C:\Program Files (x86)\Steam\steamapps\common\Control\plugins\dualsensitive-service.exe\"" /SC ONCE /ST 00:00 /RL HIGHEST /F

bool launchServerTaskOrElevated() {
    std::string taskName = "DualSensitive Service";
    if (scheduledTaskExists(taskName.c_str())) {
        std::string command("schtasks /run /TN \"" + taskName + "\" /I ");
        _LOG("Running task, command: %s", command.c_str());
        int result = WinExec(command.c_str(), SW_HIDE);
        if (result > 31){
            _LOG("Service ran successfully");
            return true;
        }
        _LOG("Running task failed (code: %d). Falling back to elevation.", result);
    } else {
        _LOG("Scheduled task not found. Falling back to elevation.");
    }

    // Final fallback
    if (!launchServerElevated()) {
        _LOG("Fallback elevation also failed. Check permissions or try manually running dualsensitive-service.exe.");
        return false;
    }

    return true;
}


bool terminateServer(PROCESS_INFORMATION& procInfo) {
    if (procInfo.hProcess != nullptr) {
        BOOL result = TerminateProcess(procInfo.hProcess, 0); // 0 = exit code
        CloseHandle(procInfo.hProcess);
        procInfo.hProcess = nullptr;
        return result == TRUE;
    }
    return false;
}


void logLastError(const char* context) {
    DWORD errorCode = GetLastError();
    wchar_t* msgBuf = nullptr;

    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        0,
        (LPWSTR)&msgBuf,
        0,
        nullptr
    );

    // Convert wchar_t* msgBuf to char*
    char narrowBuf[1024] = {};
    if (msgBuf) {
        WideCharToMultiByte(CP_UTF8, 0, msgBuf, -1, narrowBuf, sizeof(narrowBuf) - 1, nullptr, nullptr);
    }

    _LOG("[ERROR] %s failed (code %lu): %s", context, errorCode, msgBuf ? narrowBuf : "(Unknown error)");

    if (msgBuf) {
        LocalFree(msgBuf);
    }
}

struct TriggerSetting {
    TriggerProfile profile;
    bool isCustomTrigger = false;
    TriggerMode mode = TriggerMode::Off;
    std::vector<uint8_t> extras;

    TriggerSetting(TriggerProfile profile, std::vector<uint8_t> extras) :
        profile(profile), extras(extras) {}

    TriggerSetting(TriggerMode mode, std::vector<uint8_t> extras) :
        mode(mode), extras(extras), isCustomTrigger(true) {}

};

struct Triggers {
    TriggerSetting *L2;
    TriggerSetting *R2;
};

// Globals
Config g_config;
Logger g_logger;
std::vector<std::string> g_WeaponList = {
    "WEAPON_PISTOL_DEFAULT",
    "WEAPON_SHOTGUN_SINGLESHOT",
    "WEAPON_SMG_STANDARD",
    "WEAPON_RAILGUN_STANDARD",
    "WEAPON_ROCKETLAUNCHER_TRIPLESHOT",
    "WEAPON_DLC2_STICKYLAUNCHER"
};

std::vector<std::string> g_WeaponListINI = {
    "Grip",
    "Shatter",
    "Spin",
    "Pierce",
    "Charge",
    "Surge"
};

std::map<std::string, Triggers> g_TriggerSettings ;

void InitTriggerSettings() {
    g_TriggerSettings =
    {
        {
            "WEAPON_PISTOL_DEFAULT", // Grip
            {
                .L2 = new TriggerSetting(TriggerProfile::Choppy, {}),
                .R2 = new TriggerSetting(TriggerProfile::Soft, {})
            }
        },
        {
            "WEAPON_SHOTGUN_SINGLESHOT", // Shatter
            {
                .L2 = new TriggerSetting (
                        TriggerMode::Rigid_A,
                        {60, 71, 56, 128, 195, 210, 255}
                ),
                .R2 = new TriggerSetting (
                        TriggerProfile::SlopeFeedback,
                        {0, 5, 1, 8}
                )
            }
        },
        {
            "WEAPON_SMG_STANDARD", // Spin
            {
                .L2 = new TriggerSetting (
                        TriggerMode::Rigid_A,
                        {71, 96, 128, 128, 128, 128, 128}
                ),
                .R2 = new TriggerSetting(
                        TriggerProfile::Vibration,
                        {3, 4, 14}
                )
            }
        },
        {
            "WEAPON_RAILGUN_STANDARD", // Pierce
            {
                .L2 = new TriggerSetting (
                        TriggerProfile::Machine,
                        {1, 8, 3, 3, 184, 0}
                ),
                .R2 = new TriggerSetting (
                        TriggerMode::Pulse_B,
                        {238, 215, 66, 120, 43, 160, 215}
                )
            }
        },
        {
            "WEAPON_ROCKETLAUNCHER_TRIPLESHOT", // Charge
            {
                .L2 = new TriggerSetting(TriggerMode::Rigid, {}),
                .R2 = new TriggerSetting (
                        TriggerMode::Rigid_A,
                        {209, 42, 232, 192, 232, 209, 232}
                )
            }
        },
        {
            "WEAPON_DLC2_STICKYLAUNCHER", // Surge
            {
                .L2 = new TriggerSetting(TriggerProfile::Feedback, {3, 3}),
                .R2 = new TriggerSetting(TriggerProfile::VeryHard, {})
            }
        }
    };
}

void SendTriggers(std::string weaponType) {
    Triggers t = g_TriggerSettings[weaponType];
    if (t.L2->isCustomTrigger)
        dualsensitive::setLeftCustomTrigger(t.L2->mode, t.L2->extras);
    else
        dualsensitive::setLeftTrigger (t.L2->profile, t.L2->extras);
    if (t.R2->isCustomTrigger)
        dualsensitive::setRightCustomTrigger(t.R2->mode, t.R2->extras);
    else
        dualsensitive::setRightTrigger (t.R2->profile, t.R2->extras);
    _LOGD("Adaptive Trigger settings sent successfully!");
}

// Game global vars

// Use DSM_ prefix for some symbols to avoid collision with
// service weapon hotkeys mod

static DWORD mainThread = -1;

// Game functions
using _OnGameEvent_Internal =
                    void(*)(void* entityComponentState, char *eventMessage);
using _DSM_Loadout_TypeRegistration =
                    void(*)(void* a1, void* loadoutModelObject);
using _AppEventHandler =
                    bool(*)(void* EventHandler_self, void* evt);
using _DSM_InputManager_IsMenuOn =
                    bool(*)(void* inputManager);
using _InputManager_IsGameOn =
                    bool(*)(void* inputManager);
using _InputManager_SetGame =
                    void(*)(void* inputManager, bool on);
using _InputManager_SetMenu =
                    void(*)(void* inputManager, bool on);


struct ModelHandle {
    void* unk00[2];        // 00
    const char* modelName; // 10
    void** pDataStart;     // 18 - pointer to 0x28
    void* unk20;           // 20
    char data[1];          // 28
};
static_assert(offsetof(ModelHandle, data) == 0x28, "offset error");

struct FieldProperty {
    void* unk00[3];  // 00
    uint32_t offset; // 18
    //...
};

struct PropertyHandle {
    void* unk00[3];                 // 00
    FieldProperty* fieldProperty;   // 18
    //...
};

using _DSM_ModelHandle_GetPropertyHandle = PropertyHandle * (*)(
    void* modelHandle,
    void* size_20h_struct,
    const char* propertyName
);

struct InPlaceVector {
    void** items;       // 00
    uint32_t size;      // 08
    uint32_t capacity;  // 0C
    char data[1];       // 10
};

struct GameWindowVtbl {
    void* Dtor; // 00
    void* dispatchAppEvent;
    _AppEventHandler eventHandlers[11];
    _AppEventHandler GamepadButtonEvent;
    //...
};

static_assert (
    offsetof(GameWindowVtbl, GamepadButtonEvent) == 0x68, "offset error"
);

struct GamepadButtonEvent {
    uint32_t unk00;
    uint32_t buttonID;
    bool isDown;
};

// Game Addresses

// Offset of the GIDEntity (first) and GameInventoryComponentState (second)
// from ModelHandle
RVA<uintptr_t>
DSM_Addr_Entry_GIDEntity_and_ModelHandle_GameInventoryComponentState_Offset (
    "48 8B 83 ? ? ? ? 45 8B CC 48 8B 8E ? ? ? ? 48 89 84 24"
);

// Size of each weapon entry in the model
RVA<uintptr_t>
DSM_Addr_WeaponEntrySize (
    "48 69 C0 ? ? ? ? 4C 63 E2 48 03 C7"
);

// Offset of currently equipped weapon from GameInventoryComponentState
RVA<uintptr_t>
DSM_Addr_GameInventoryComponentState_EquippedWeaponOffset (
    "48 8B 89 ? ? ? ? 48 39 08 74 0C"
);

// From coherentuigt, to retrieve a property handle from a model handle by name
_DSM_ModelHandle_GetPropertyHandle DSM_ModelHandle_GetPropertyHandle = nullptr;

// Hooks

// We hook this one to receive loadoutModelHandle + 0x18 in arg2
RVA<_DSM_Loadout_TypeRegistration> DSM_Loadout_TypeRegistration_Internal (
    "E8 ? ? ? ? 48 8B D7 48 8B CB FF 15 ? ? ? ? 4D 8B 0E 48 8D 15 ? ? ? ?", 0, 1, 5
);
_DSM_Loadout_TypeRegistration DSM_Loadout_TypeRegistration_Original = nullptr;

// This function is called every time a game event takes place. We purposely
// avoid hooking an equip weapon function to avoid conflicts with other mods,
// like the weapon switching one
RVA<_OnGameEvent_Internal>
OnGameEvent_Internal (
    //"4c 8b dc 57 48 81 ec a0 ? ? ? 48 c7 44 24 20 fe ? ? ? 49 89 5b 18 48 8b 05"
    "4c 8b dc 57 48 81 ec a0 00 00 00 48 c7 44 24 20 fe ff ff ff 49 89 5b 18 48 8b 05"
);
_OnGameEvent_Internal OnGameEvent_Original = nullptr;
// from input - InputManager is created quite early, before startvideos.tex
// plays
void** DSM_InputManager_ppInstance = nullptr;
// from input, to determine whether a menu
// (e.g. inventory / conversation / fast travel) is active
_DSM_InputManager_IsMenuOn DSM_InputManager_IsMenuOn = nullptr;
// to skip equip weapon events when we are on a non-game screen
_InputManager_IsGameOn InputManager_IsGameOn = nullptr;
// to turn off adaptive triggers when not in game
_InputManager_SetGame InputManager_SetGame_Internal = nullptr;
_InputManager_SetGame InputManager_SetGame_Original = nullptr;
// to turn off adaptive triggers when on player menu
_InputManager_SetMenu InputManager_SetMenu_Internal = nullptr;
_InputManager_SetMenu InputManager_SetMenu_Original = nullptr;

// Offsets / Struct Sizes
int g_WeaponEntrySize = 0;
int g_WeaponEntry_GIDEntity_Offset = 0;
int g_ModelHandle_GameInventoryComponentState_Offset = 0;
int g_GameInventoryComponentState_EquippedWeaponOffset = 0;

// Globals
ModelHandle* g_loadoutModelHandle = nullptr;
// This is needed because there is a small race between the DSX heartbeat
// thread and the OnGameEvent hook
std::mutex g_currentWeaponMutex;
std::string g_currentWeaponName;

// This is to make sure that the PID will be sent to the server after
// the server has started
std::mutex g_serverLaunchMutex;
bool g_serverStarted = false;

bool g_recharging = false;

HMODULE GetRMDModule(const char* modName) {
    char szModuleName[MAX_PATH] = "";
    snprintf(szModuleName, sizeof(szModuleName), "%s_rmdwin7_f.dll", modName);
    HMODULE hMod = GetModuleHandleA(szModuleName);

    if (!hMod) {
        snprintf(szModuleName, sizeof(szModuleName), "%s_rmdwin10_f.dll", modName);
        hMod = GetModuleHandleA(szModuleName);
    }

    if (!hMod) {
        _LOG("WARNING: Could not get module: %s.", modName);
    }
    return hMod;
}

namespace DualsenseMod {

    // Read and populate offsets and addresses from game code
    bool PopulateOffsets() {
        g_WeaponEntry_GIDEntity_Offset = *reinterpret_cast<uint32_t *> (
            DSM_Addr_Entry_GIDEntity_and_ModelHandle_GameInventoryComponentState_Offset.GetUIntPtr() + 3
        );
        g_ModelHandle_GameInventoryComponentState_Offset = *reinterpret_cast<uint32_t *> (
            DSM_Addr_Entry_GIDEntity_and_ModelHandle_GameInventoryComponentState_Offset.GetUIntPtr() + 13
        );
        g_WeaponEntrySize = *reinterpret_cast<uint32_t *> (
            DSM_Addr_WeaponEntrySize.GetUIntPtr() + 3
        );
        g_GameInventoryComponentState_EquippedWeaponOffset = *reinterpret_cast<uint32_t *> (
            DSM_Addr_GameInventoryComponentState_EquippedWeaponOffset.GetUIntPtr() + 3
        );

        HMODULE hMod = GetModuleHandleA("coherentuigt.dll");
        DSM_ModelHandle_GetPropertyHandle =
            (_DSM_ModelHandle_GetPropertyHandle) GetProcAddress (
                hMod,
                "?GetPropertyHandle@ModelHandle@UIGT@Coherent@@QEBA?AUPropertyHandle@23@PEBD@Z"
            );

        HMODULE hInput = GetRMDModule("input");
        DSM_InputManager_ppInstance = (void**) GetProcAddress (
            hInput,
            "?sm_pInstance@InputManager@input@@0PEAV12@EA"
        );
        DSM_InputManager_IsMenuOn = (_DSM_InputManager_IsMenuOn) GetProcAddress (
            hInput,
            "?isMenuOn@InputManager@input@@QEAA_NXZ"
        );
        InputManager_IsGameOn = (_InputManager_IsGameOn) GetProcAddress (
            hInput,
            "?isGameOn@InputManager@input@@QEAA_NXZ"
        );
        InputManager_SetGame_Internal = (_InputManager_SetGame) GetProcAddress (
            hInput,
            "?setGame@InputManager@input@@QEAAX_N@Z"
        );
        InputManager_SetMenu_Internal = (_InputManager_SetMenu) GetProcAddress (
            hInput,
            "?setMenu@InputManager@input@@QEAAX_N@Z"
        );

        _LOG("Offsets: %X, %X, %X, %X",
            g_WeaponEntry_GIDEntity_Offset,
            g_ModelHandle_GameInventoryComponentState_Offset,
            g_WeaponEntrySize, g_GameInventoryComponentState_EquippedWeaponOffset
        );
        _LOG("OnGameEvent_Internal at %p",
            OnGameEvent_Internal.GetUIntPtr()
        );
        _LOG("DSM_ModelHandle_GetPropertyHandle at %p",
            DSM_ModelHandle_GetPropertyHandle
        );
        _LOG("InputManager_pInstance at %p",
            DSM_InputManager_ppInstance
        );
        _LOG("DSM_InputManager_IsMenuOn at %p",
            DSM_InputManager_IsMenuOn
        );
        _LOG("InputManager_IsGameOn at %p",
            InputManager_IsGameOn
        );
        _LOG("InputManager_SetGame_Internal at %p",
            InputManager_SetGame_Internal
        );
        _LOG("InputManager_SetMenu_Internal at %p",
            InputManager_SetMenu_Internal
        );

        if (!DSM_ModelHandle_GetPropertyHandle || !DSM_InputManager_ppInstance || !DSM_InputManager_IsMenuOn || !InputManager_IsGameOn || !InputManager_SetGame_Internal || !InputManager_SetMenu_Internal)
            return false;

        return true;
    }

    void setAdaptiveTriggersForCurrrentWeapon();
    void DSM_Loadout_TypeRegistration_Hook(void *arg1, void *loadoutModelObject) {
        if (!g_loadoutModelHandle) {
            g_loadoutModelHandle =
                    (ModelHandle *) ((char *)loadoutModelObject - 0x18);
            _LOG("g_loadoutModelHandle: %p", g_loadoutModelHandle);
            DWORD threadId = GetCurrentThreadId();
            _LOG("Main thread id: %d", threadId);
            mainThread = threadId;
        }
        DSM_Loadout_TypeRegistration_Original(arg1, loadoutModelObject);
    }

    void* getModelProperty(const char* propertyName) {
        if (!g_loadoutModelHandle)
            return nullptr;
        char arg2[0x20];
        auto propertyHandle = DSM_ModelHandle_GetPropertyHandle (
            g_loadoutModelHandle,
            arg2,
            propertyName
        );
        if (!propertyHandle)
            return nullptr;
        _LOGD("getModelProperty: Computed offset for %s is 0x%x.",
            propertyName,
            propertyHandle->fieldProperty->offset
        );
        return (char *) &g_loadoutModelHandle->pDataStart +
                                    propertyHandle->fieldProperty->offset;
    }

    // Use memoization
    std::unordered_map<uint64_t, std::string> memo;
    const char *getWeaponName(uint64_t gid) {
        if (memo.find(gid) != memo.end()) {
            return memo[gid].c_str();
        }
        // NOTE: Loadout model must be valid
        InPlaceVector *vecWeapons =
            (InPlaceVector *) getModelProperty("m_vecEquippedWeapons");
        if (!vecWeapons)
            return nullptr;
        void* weaponItem = vecWeapons->items;
        for (unsigned int i = 0; i < vecWeapons->size; i++) {
            uint64_t weaponId = Utils::GetOffset<uint64_t> (
                weaponItem,
                g_WeaponEntry_GIDEntity_Offset
            );
            if (weaponId == gid) {
                memo[gid] = std::string (
                    Utils::GetOffset<char*>(weaponItem, 0x10)
                );
                return memo[gid].c_str();
            }
            weaponItem = (char *) weaponItem + g_WeaponEntrySize;
        }
        return nullptr;
    }

    inline uint64_t getCurrentlyEquippedWeaponId() {
        void *gameInventoryComponentState = Utils::GetOffset<void *> (
            g_loadoutModelHandle,
            g_ModelHandle_GameInventoryComponentState_Offset
        );
        uint64_t equippedWeaponId = Utils::GetOffset<uint64_t> (
            gameInventoryComponentState,
            g_GameInventoryComponentState_EquippedWeaponOffset
        );
        return equippedWeaponId;
    }

    void resetAdaptiveTriggers() {
        // TODO: do that in a separate thread to avoid stuttering
        // reset triggers to Normal mode
        dualsensitive::setLeftTrigger(TriggerProfile::Normal);
        dualsensitive::setRightTrigger(TriggerProfile::Normal);
        _LOGD("Adaptive Triggers reset successfully!");
    }

    void setAdaptiveTriggersForCurrrentWeapon() {
        if (!g_loadoutModelHandle) {
            _LOGD("loadout not set yet, skipping...");
            return;
        }
        uint64_t weaponId = getCurrentlyEquippedWeaponId();
        if (weaponId == 0) {
            _LOGD("* current weapon Id was zero; skipping...");
            return;
        }
        const char* weaponName = getWeaponName(weaponId);
        if (weaponName == nullptr) {
            _LOGD("* current weapon is null!");
            return;
        }
        _LOGD("* Set adaptive triggers for current weapon: %s", weaponName);
        g_currentWeaponMutex.lock();
        g_currentWeaponName = std::string(weaponName);
        SendTriggers(g_currentWeaponName);
        g_currentWeaponMutex.unlock();
    }

    void replayLatestAdaptiveTriggers() {
        g_currentWeaponMutex.lock();
        if (!g_currentWeaponName.empty()) {
            _LOGD("* Replaying latest adaptive trigger setting!");
            SendTriggers(g_currentWeaponName);
        }
        g_currentWeaponMutex.unlock();
    }

    void OnGameEvent_Hook(void* entityComponentState, char *eventMessage) {
        OnGameEvent_Original(entityComponentState, eventMessage);
        bool isGameOn = InputManager_IsGameOn(*DSM_InputManager_ppInstance);
        if (!isGameOn) {
            _LOGD("We are NOT on a Game screen, just skipping...");
            return;
        }
        bool isMenuOn = DSM_InputManager_IsMenuOn(*DSM_InputManager_ppInstance);
        if (isMenuOn) {
            _LOGD("We are on a menu screen, just skipping...");
            return;
        }
        if (eventMessage == nullptr) {
            _LOGD("event message is null!");
            return;
        }
        _LOGD("in OnGameEvent hook! - event message: \"%s\"", eventMessage);

        if (!g_recharging && !strcmp(eventMessage, "reloading_deficit_enter")) {
            _LOGD("reloading_deficit_enter found: reset adaptive triggers and enter recharging mode!");
            resetAdaptiveTriggers();
            g_recharging = true;
            return;
        }


        if (g_recharging && !strcmp(eventMessage, "reloading_deficit_exit")) {
            _LOGD("reloading_deficit_exit found: exit recharging mode!");
            g_recharging = false;
            setAdaptiveTriggersForCurrrentWeapon();
            return;
        }

        // case of entring the game screen for the first time
        if (g_currentWeaponName.empty() &&
                !strcmp(eventMessage, "unholster_end")) {
            _LOG("Entered the Game!");
            setAdaptiveTriggersForCurrrentWeapon();
        }

        if (strcmp(eventMessage, "weapon_equipped")) {
            return;
        }


        if (!g_recharging) {
            setAdaptiveTriggersForCurrrentWeapon();
        }
    }


    void InputManager_SetGame_Hook (void* inputManager, bool on) {
        InputManager_SetGame_Original(inputManager, on);
        _LOGD("In InputManager::SetGame hook, on: %s", on ? "true" : "false");
        if (!on) {
            _LOGD(" * (pause menu) turn off adaptive triggers!");
            resetAdaptiveTriggers();
            return;
        }
        // on: enable adaptive triggers again
        _LOGD(" * (in game again!) turn on adaptive triggers!");
        if (!g_recharging) {
            replayLatestAdaptiveTriggers();
        }
    }

    void InputManager_SetMenu_Hook (void *inputManager, bool on) {
        InputManager_SetMenu_Original(inputManager, on);
        _LOGD("In InputManager::SetMenu hook, on: %s", on ? "true" : "false");
        if (on) {
            _LOGD(" * (Player menu) turn off adaptive triggers!");
            resetAdaptiveTriggers();
            return;
        }
        bool isGameOn = InputManager_IsGameOn(*DSM_InputManager_ppInstance);
        if (isGameOn) {
            _LOGD(" * (in game again!) turn on adaptive triggers!");

            if (!g_recharging) {
                setAdaptiveTriggersForCurrrentWeapon();
            }
        }
    }

    bool ApplyHooks() {
        _LOG("Applying hooks...");
        // Hook loadout type registration to obtain pointer to the model handle
        MH_Initialize();

        MH_CreateHook (
            DSM_Loadout_TypeRegistration_Internal,
            DSM_Loadout_TypeRegistration_Hook,
            reinterpret_cast<LPVOID *>(&DSM_Loadout_TypeRegistration_Original)
        );
        if (MH_EnableHook(DSM_Loadout_TypeRegistration_Internal) != MH_OK) {
            _LOG("FATAL: Failed to install hook.");
            return false;
        }
        // Hook an internal function that called on various game events to
        // configure Dualsense adaptive triggers when switching weapons
        MH_CreateHook (
            OnGameEvent_Internal,
            OnGameEvent_Hook,
            reinterpret_cast<LPVOID *>(&OnGameEvent_Original)
        );
        if (MH_EnableHook(OnGameEvent_Internal) != MH_OK) {
            _LOG("FATAL: Failed to install OnGameEvent hook.");
            return false;
        }

        // Hook
        MH_CreateHook (
            InputManager_SetMenu_Internal,
            InputManager_SetMenu_Hook,
            reinterpret_cast<LPVOID *>(&InputManager_SetMenu_Original)
        );
        if (MH_EnableHook(InputManager_SetMenu_Internal) != MH_OK) {
            _LOG("FATAL: Failed to install InputManager::SetMenu hook.");
            return false;
        }

        // Hook
        MH_CreateHook (
            InputManager_SetGame_Internal,
            InputManager_SetGame_Hook,
            reinterpret_cast<LPVOID *>(&InputManager_SetGame_Original)
        );
        if (MH_EnableHook(InputManager_SetGame_Internal) != MH_OK) {
            _LOG("FATAL: Failed to install InputManager::SetGame hook.");
            return false;
        }

        _LOG("Hooks applied successfully!");

        return true;
    }

    bool InitAddresses() {
        _LOG("Sigscan start");
        RVAUtils::Timer tmr; tmr.start();
        RVAManager::UpdateAddresses(0);
        _LOG("Sigscan elapsed: %llu ms.", tmr.stop());

        // Check if all addresses were resolved
        for (auto rvaData : RVAManager::GetAllRVAs()) {
            if (!rvaData->effectiveAddress) {
                _LOG("Signature: %s was not resolved!", rvaData->sig);
            }
        }
        if (!RVAManager::IsAllResolved())
            return false;

        return true;
    }


#define DEVICE_ENUM_INFO_SZ 16
#define CONTROLLER_LIMIT 16
std::string wstring_to_utf8(const std::wstring& ws) {
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1,
                                   nullptr, 0, nullptr, nullptr);
    std::string s(len, 0);
    WideCharToMultiByte (
            CP_UTF8, 0, ws.c_str(), -1, &s[0], len, nullptr, nullptr
    );
    s.resize(len - 1);
    return s;
}

    constexpr int MAX_ATTEMPTS = 5;
    constexpr int RETRY_DELAY_MS = 2000;

    void Init() {
        g_logger.Open("./plugins/dualsensemod.log");
        _LOG("DualsenseMod v1.0 by Thanos Petsas (SkyExplosionist)");
        _LOG("Game version: %" PRIX64, Utils::GetGameVersion());
        _LOG("Module base: %p", GetModuleHandle(NULL));

        // Sigscan
        if (!InitAddresses() || !PopulateOffsets()) {
            MessageBoxA (
                NULL,
                "DualsenseMod is not compatible with this version of Control.\nPlease visit the mod page for updates.",
                "DualsenseMod",
                MB_OK | MB_ICONEXCLAMATION
            );
            _LOG("FATAL: Incompatible version");
            return;
        }

        _LOG("Addresses set");

        // init config
        g_config = Config(INI_LOCATION);
        g_config.print();

        InitTriggerSettings();

        ApplyHooks();

        CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
            _LOG("Client starting DualSensitive Service...\n");

            if (!launchServerTaskOrElevated()) {
                _LOG("Error launching the DualSensitive Service...\n");
                return 1;
            }
            g_serverLaunchMutex.lock();
            g_serverStarted = true;
            g_serverLaunchMutex.unlock();
            _LOG("DualSensitive Service launched successfully...\n");
            return 0;
        }, nullptr, 0, nullptr);


        CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
            // wait for server to start first
            do {
                g_serverLaunchMutex.lock();
                bool started = g_serverStarted;
                g_serverLaunchMutex.unlock();
                if (started) break;
                std::this_thread::sleep_for(std::chrono::seconds(2));
            } while (true);

            _LOG("Client starting DualSensitive Service...\n");
            auto status = dualsensitive::init(AgentMode::CLIENT, "./plugins/DualSensitive/duaslensitive-client.log", g_config.isDebugMode);
            if (status != dualsensitive::Status::Ok) {
                _LOG("Failed to initialize DualSensitive in CLIENT mode, status: %d", static_cast<std::underlying_type<dualsensitive::Status>::type>(status));
            }
                _LOG("DualSensitive Service launched successfully...\n");
            dualsensitive::sendPidToServer();
            return 0;
        }, nullptr, 0, nullptr);

        _LOG("Ready.");
    }
}
