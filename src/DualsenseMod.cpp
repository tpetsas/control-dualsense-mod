#include "DualsenseMod.h"

#include "Logger.h"
#include "Config.h"
#include "Utils.h"
#include "rva/RVA.h"
#include "minhook/include/MinHook.h"

#include "DSX++.h"

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

#define INI_LOCATION "./plugins/dualsense-mod.ini"

struct TriggerSetting {
    TriggerMode mode;
    bool isCustomTrigger = false;
    CustomTriggerValueMode customMode = OFF;
    std::vector<int> extras;

    TriggerSetting(TriggerMode mode, std::vector<int> extras) :
        mode(mode), extras(extras) {}

    TriggerSetting(CustomTriggerValueMode customMode, std::vector<int> extras) :
        customMode(customMode), extras(extras), isCustomTrigger(true),
        mode(CustomTriggerValue) {}

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
                .L2 = new TriggerSetting(Choppy, {}),
                .R2 = new TriggerSetting(Soft, {}),
            }
        },
        {
            "WEAPON_SHOTGUN_SINGLESHOT", // Shatter
            {
                .L2 = new TriggerSetting (
                        RigidA,
                        {60, 71, 56, 128, 195, 210, 256}
                ),
                .R2 = new TriggerSetting(Hardest, {})
            }
        },
        {
            "WEAPON_SMG_STANDARD", // Spin
            {
                .L2 = new TriggerSetting (
                        RigidA,
                        {71, 96, 128, 128, 128, 128, 128}
                ),
#ifdef DSX_V3
                .R2 = new TriggerSetting(Vibration, {3, 4, 14})
#else // DSC_V2
                .R2 = new TriggerSetting(AutomaticGun, {1, 7, 6})
#endif
            }
        },
        {
            "WEAPON_RAILGUN_STANDARD", // Pierce
            {
                .L2 = new TriggerSetting(Machine, {1, 8, 3, 3, 184}),
                .R2 = new TriggerSetting (
                        VibrateResistanceB,
                        {238, 215, 66, 120, 43, 160, 215}
                )
            }
        },
        {
            "WEAPON_ROCKETLAUNCHER_TRIPLESHOT", // Charge
            {
                //.L2 = new TriggerSetting(Rigid, {}),
                .L2 = new TriggerSetting (
                        RigidA,
                        {209, 42, 232, 192, 232, 209, 232}
                ),
                .R2 = new TriggerSetting (
                        RigidA,
                        {209, 42, 232, 192, 232, 209, 232}
                )
            }
        },
        {
            "WEAPON_DLC2_STICKYLAUNCHER", // Surge
            {
#ifdef DSX_V3
                .L2 = new TriggerSetting(Feedback, {3, 3}),
#else // DSC_V2
                .L2 = new TriggerSetting(Rigid, {})
#endif
                .R2 = new TriggerSetting(VeryHard, {})
            }
        }
    };
}

void SendTriggers(std::string weaponType) {
    Triggers t = g_TriggerSettings[weaponType];
    if (t.L2->isCustomTrigger)
        DSX::setLeftCustomTrigger (t.L2->customMode, t.L2->extras);
    else
        DSX::setLeftTrigger (t.L2->mode, t.L2->extras);
    if (t.R2->isCustomTrigger)
        DSX::setRightCustomTrigger (t.R2->customMode, t.R2->extras);
    else
        DSX::setRightTrigger (t.R2->mode, t.R2->extras);
    if (DSX::sendPayload() != DSX::Success) {
        _LOG("DSX++ client failed to send data!");
        return;
    }
    _LOG("Addaptive Trigger settings sent successfully!");
}

// Game global vars

static DWORD mainThread = -1;

// Game functions
using _OnGameEvent_Internal =
                    void(*)(void* entityComponentState, char *eventMessage);
using _Loadout_TypeRegistration =
                    void(*)(void* a1, void* loadoutModelObject);
using _AppEventHandler =
                    bool(*)(void* EventHandler_self, void* evt);
using _InputManager_IsMenuOn =
                    bool(*)(void* inputManager);
using _InputManager_IsLoadingOn =
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

using _ModelHandle_GetPropertyHandle = PropertyHandle * (*)(
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
Addr_Entry_GIDEntity_and_ModelHandle_GameInventoryComponentState_Offset (
    "48 8B 83 ? ? ? ? 45 8B CC 48 8B 8E ? ? ? ? 48 89 84 24"
);

// Size of each weapon entry in the model
RVA<uintptr_t>
Addr_WeaponEntrySize (
    "48 69 C0 ? ? ? ? 4C 63 E2 48 03 C7"
);

// Offset of currently equipped weapon from GameInventoryComponentState
RVA<uintptr_t>
Addr_GameInventoryComponentState_EquippedWeaponOffset (
    "48 8B 89 ? ? ? ? 48 39 08 74 0C"
);

// savegame call inside OnWeaponEquipped
RVA<uintptr_t>
OnWeaponEquipped_SaveGame_Addr (
    "33 D2 45 33 C0 8D 4A 02 FF 15 ? ? ? ? 4C 8D 05 ? ? ? ?", 8
);

// From coherentuigt, to retrieve a property handle from a model handle by name
_ModelHandle_GetPropertyHandle ModelHandle_GetPropertyHandle = nullptr;

// Hooks

// We hook this one to receive loadoutModelHandle + 0x18 in arg2
RVA<_Loadout_TypeRegistration> Loadout_TypeRegistration_Internal (
    "E8 ? ? ? ? 48 8B D7 48 8B CB FF 15 ? ? ? ? 4D 8B 0E 48 8D 15 ? ? ? ?", 0, 1, 5
);
_Loadout_TypeRegistration Loadout_TypeRegistration_Original = nullptr;

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
void** InputManager_ppInstance = nullptr;
// from input, to determine whether a menu
// (e.g. inventory / conversation / fast travel) is active
_InputManager_IsMenuOn InputManager_IsMenuOn = nullptr;
// to skip equip weapon events when we are on a loading screen
_InputManager_IsLoadingOn InputManager_IsLoadingOn = nullptr;
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
            Addr_Entry_GIDEntity_and_ModelHandle_GameInventoryComponentState_Offset.GetUIntPtr() + 3
        );
        g_ModelHandle_GameInventoryComponentState_Offset = *reinterpret_cast<uint32_t *> (
            Addr_Entry_GIDEntity_and_ModelHandle_GameInventoryComponentState_Offset.GetUIntPtr() + 13
        );
        g_WeaponEntrySize = *reinterpret_cast<uint32_t *> (
            Addr_WeaponEntrySize.GetUIntPtr() + 3
        );
        g_GameInventoryComponentState_EquippedWeaponOffset = *reinterpret_cast<uint32_t *> (
            Addr_GameInventoryComponentState_EquippedWeaponOffset.GetUIntPtr() + 3
        );

        HMODULE hMod = GetModuleHandleA("coherentuigt.dll");
        ModelHandle_GetPropertyHandle =
            (_ModelHandle_GetPropertyHandle) GetProcAddress (
                hMod,
                "?GetPropertyHandle@ModelHandle@UIGT@Coherent@@QEBA?AUPropertyHandle@23@PEBD@Z"
            );

        HMODULE hInput = GetRMDModule("input");
        InputManager_ppInstance = (void**) GetProcAddress (
            hInput,
            "?sm_pInstance@InputManager@input@@0PEAV12@EA"
        );
        InputManager_IsMenuOn = (_InputManager_IsMenuOn) GetProcAddress (
            hInput,
            "?isMenuOn@InputManager@input@@QEAA_NXZ"
        );
        InputManager_IsLoadingOn = (_InputManager_IsMenuOn) GetProcAddress (
            hInput,
            "?isLoadingOn@InputManager@input@@QEAA_NXZ"
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
        _LOG("ModelHandle_GetPropertyHandle at %p",
            ModelHandle_GetPropertyHandle
        );
        _LOG("InputManager_pInstance at %p",
            InputManager_ppInstance
        );
        _LOG("InputManager_IsMenuOn at %p",
            InputManager_IsMenuOn
        );
        _LOG("InputManager_IsLoadingOn at %p",
            InputManager_IsLoadingOn
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

        if (!ModelHandle_GetPropertyHandle || !InputManager_ppInstance || !InputManager_IsMenuOn || !InputManager_IsLoadingOn || !InputManager_IsGameOn || !InputManager_SetGame_Internal || !InputManager_SetMenu_Internal)
            return false;

        return true;
    }

    void setAdaptiveTriggersForCurrrentWeapon();
    void Loadout_TypeRegistration_Hook(void *arg1, void *loadoutModelObject) {
        if (!g_loadoutModelHandle) {
            g_loadoutModelHandle =
                    (ModelHandle *) ((char *)loadoutModelObject - 0x18);
            _LOG("g_loadoutModelHandle: %p", g_loadoutModelHandle);
            DWORD threadId = GetCurrentThreadId();
            _LOG("Main thread id: %d", threadId);
            mainThread = threadId;
        }
        Loadout_TypeRegistration_Original(arg1, loadoutModelObject);
    }

    void* getModelProperty(const char* propertyName) {
        if (!g_loadoutModelHandle)
            return nullptr;
        char arg2[0x20];
        auto propertyHandle = ModelHandle_GetPropertyHandle (
            g_loadoutModelHandle,
            arg2,
            propertyName
        );
        if (!propertyHandle)
            return nullptr;
        _LOG("getModelProperty: Computed offset for %s is 0x%x.",
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
        // TODO: do that in a separate thread to aboid stuttering
        // reset triggers to Normal mode
        DSX::setLeftTrigger (Normal);
        DSX::setRightTrigger (Normal);
        if (DSX::sendPayload() != DSX::Success) {
            _LOG("DSX++ client failed to send data!");
            return;
        }
        _LOG("Addaptive Triggers reset successfully!");
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
        _LOG("* Set adaptive triggers for current weapon: %s", weaponName);
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
        _LOGD("in OnGameEvent hook! - event message: \"%s\"", eventMessage);
        bool isGameOn = InputManager_IsGameOn(*InputManager_ppInstance);
        if (!isGameOn) {
            _LOGD("We are NOT on a Game screen, just skipping...");
            return;
        }
        bool isMenuOn = InputManager_IsMenuOn(*InputManager_ppInstance);
        if (isMenuOn) {
            _LOGD("We are on a menu screen, just skipping...");
            return;
        }
#if 0
        bool isLoadingOn = InputManager_IsLoadingOn(*InputManager_ppInstance);
        if (isLoadingOn) {
            _LOGD("We are on a loading screen, just skipping...");
            return;
        }
#endif
        if (eventMessage == nullptr) {
            _LOGD("event message is null!");
            return;
        }
        _LOGD("in OnGameEvent hook! - event message: \"%s\"", eventMessage);

        // case of entring the game screen for the first time
        if (g_currentWeaponName.empty() &&
                !strcmp(eventMessage, "unholster_end")) {
            _LOG("Entered the Game!");
            setAdaptiveTriggersForCurrrentWeapon();
        }

        if (strcmp(eventMessage, "weapon_equipped")) {
            return;
        }
        setAdaptiveTriggersForCurrrentWeapon();
    }


    void InputManager_SetGame_Hook (void* inputManager, bool on) {
        InputManager_SetGame_Original(inputManager, on);
        _LOG("In InputManager::SetGame hook, on: %s", on ? "true" : "false");
        if (!on) {
            _LOG(" * (pause menu) turn off adaptive triggers!");
            resetAdaptiveTriggers();
            return;
        }
        // on: enable adaptive triggers again
        _LOG(" * (in game again!) turn on adaptive triggers!");
        replayLatestAdaptiveTriggers();
    }

    void InputManager_SetMenu_Hook (void *inputManager, bool on) {
        InputManager_SetMenu_Original(inputManager, on);
        _LOG("In InputManager::SetMenu hook, on: %s", on ? "true" : "false");
        if (on) {
            _LOG(" * (Player menu) turn off adaptive triggers!");
            resetAdaptiveTriggers();
            return;
        }
        bool isGameOn = InputManager_IsGameOn(*InputManager_ppInstance);
        if (isGameOn) {
            _LOG(" * (in game again!) turn on adaptive triggers!");
            replayLatestAdaptiveTriggers();
        }
    }

    bool ApplyHooks() {
        _LOG("Applying hooks...");
        // Hook loadout type registration to obtain pointer to the model handle
        MH_Initialize();
        MH_CreateHook (
            Loadout_TypeRegistration_Internal,
            Loadout_TypeRegistration_Hook,
            reinterpret_cast<LPVOID *>(&Loadout_TypeRegistration_Original)
        );
        if (MH_EnableHook(Loadout_TypeRegistration_Internal) != MH_OK) {
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

    // we need to spin up a thread and replay the latest adaptive triggers
    // settings every 30 seconds, otherwise the DSX settings will get lost
    // if the player stays idle for a while
    DWORD WINAPI SendHearbeatToDSX(LPVOID lpParam) {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(30));

            bool isGameOn = InputManager_IsGameOn(*InputManager_ppInstance);
            if (!isGameOn) {
                _LOGD("We are NOT on a Game screen, just skipping...");
                continue;
            }
            bool isMenuOn = InputManager_IsMenuOn(*InputManager_ppInstance);
            if (isMenuOn) {
                _LOGD("We are on a menu screen, just skipping...");
                continue;
            }
            replayLatestAdaptiveTriggers();
        }
    }

    void Init() {
#if 0
        char logPath[MAX_PATH];
        if (SUCCEEDED (
                SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, NULL, logPath)
            )) {
            strcat_s(logPath, "\\Remedy\\Control\\WeaponSwitch.log");
            g_logger.Open(logPath);
        }
#endif
        // this should be first in order to be able to log
        g_logger.Open("./plugins/modlog.log");
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

        // Disable the savegame call that happens when equipping weapons as this causes stutter and player may switch weapons often
        unsigned char data[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
        Utils::WriteMemory(OnWeaponEquipped_SaveGame_Addr.GetUIntPtr(), data, sizeof(data));

        CreateThread(NULL, 0, SendHearbeatToDSX, NULL, 0, NULL);

        if ( DSX::init() != DSX::Success ) {
            _LOG("DSX++ client failed to initialize!");
            return;
        }
        _LOG("DSX++ client initialized successfully!");

        _LOG("Ready.");
    }
}
