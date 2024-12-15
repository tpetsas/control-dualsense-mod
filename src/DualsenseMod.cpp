#include "DualsenseMod.h"

#include "Logger.h"
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

#define INI_LOCATION "./plugins/DualsenseMod.ini"

struct TriggerSetting {
    TriggerMode mode;
    bool isCustomTrigger = false;
    CustomTriggerValueMode customMode = OFF;
    std::vector<int> extras;

    TriggerSetting(TriggerMode mode, std::vector<int> extras) :
        mode(mode), extras(extras) {}

};

struct Triggers {
    TriggerSetting L2;
    TriggerSetting R2;
};

// Globals
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
            "WEAPON_PISTOL_DEFAULT",
            {
                .L2 = TriggerSetting(Choppy, {}),
                .R2 = TriggerSetting(Soft, {}),
            }
        },
        {
            "WEAPON_SHOTGUN_SINGLESHOT",
            {
                .L2 = TriggerSetting(Rigid, {}),
                .R2 = TriggerSetting(Medium, {})
            }
        },
    };
}
#if 0
void SendTriggers(std::string weaponType) {
    Triggers t = g_TriggerSettings[weaponType];
    DSX::setLeftTrigger (t.L2.mode, t.L2.extras);
    DSX::setRightTrigger (t.R2.mode, t.R2.extras);
    if (DSX::sendPayload() != DSX::Success) {
        _LOG("DSX++ client failed to send data!");
        return;
    }
    _LOG("Addaptive Trigger settings sent successfully!");
}
#endif

// Game functions
using _OnGameEvent_Internal =
                    void(*)(void* entityComponentState, char *event_message);
using _Loadout_TypeRegistration =
                    void(*)(void* a1, void* loadoutModelObject);
using _AppEventHandler =
                    bool(*)(void* EventHandler_self, void* evt);

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
    "4c 8b dc 57 48 81 ec a0 ? ? ? 48 c7 44 24 20 fe ? ? ? 49 89 5b 18 48 8b 05"
);
_OnGameEvent_Internal OnGameEvent_Original = nullptr;


// Offsets / Struct Sizes
int g_WeaponEntrySize = 0;
int g_WeaponEntry_GIDEntity_Offset = 0;
int g_ModelHandle_GameInventoryComponentState_Offset = 0;
int g_GameInventoryComponentState_EquippedWeaponOffset = 0;

// Globals
ModelHandle* g_loadoutModelHandle = nullptr;

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

        if (!ModelHandle_GetPropertyHandle)
            return false;

        return true;
    }

    void Loadout_TypeRegistration_Hook(void *arg1, void *loadoutModelObject) {
        if (!g_loadoutModelHandle) {
            g_loadoutModelHandle =
                    (ModelHandle *) ((char *)loadoutModelObject - 0x18);
            _LOG("g_loadoutModelHandle: %p", g_loadoutModelHandle);
        }
        Loadout_TypeRegistration_Original(arg1, loadoutModelObject);
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
            return NULL;
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
        return NULL;
    }

    void OnGameEvent_Hook(void* entityComponentState, char *event_message) {
        OnGameEvent_Original(entityComponentState, event_message);
        if (strcmp(event_message, "weapon_equipped"))
            return;
        _LOG("in OnGameEvent hook! - event message: \"%s\"", event_message);

        if (!g_loadoutModelHandle) {
            _LOG("loadout not set yet, skipping...");
            return;
        }
        uint64_t weaponId = getCurrentlyEquippedWeaponId();
        const char* weaponName = getWeaponName(weaponId);
        _LOG("* current weapon: %s", weaponName);
    }


    bool Hook() {
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
            _LOG("FATAL: Failed to install OnGameEvent_Internal hook.");
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


    DWORD WINAPI sendHearbeatToDSX(LPVOID lpParam) {
        // TODO
        _LOG("FATAL: Failed to find DSX Server.");
        MessageBoxA (
            NULL,
            "Failed to find DSX Server",
            "Warning",
            MB_OK | MB_ICONEXCLAMATION
        );
        return 0;
    };


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

        Hook();

        // TODO
        //CreateThread(NULL, 0, sendHearbeatToDSX, NULL, 0, NULL);

        if ( DSX::init() != DSX::Success ) {
            _LOG("DSX++ client failed to initialize!");
            return;
        }
        _LOG("DSX++ client initialized successfully!");

        _LOG("Ready.");
    }
}
