#include "DualsenseMod.h"

#include "Logger.h"
#include "Utils.h"
#include "rva/RVA.h"
#include "minhook/include/MinHook.h"

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

//===============
// Globals
//===============
Logger g_logger;
std::vector<std::string> g_WeaponList = { "WEAPON_PISTOL_DEFAULT", "WEAPON_SHOTGUN_SINGLESHOT", "WEAPON_SMG_STANDARD", "WEAPON_RAILGUN_STANDARD", "WEAPON_ROCKETLAUNCHER_TRIPLESHOT", "WEAPON_DLC2_STICKYLAUNCHER" };
std::vector<std::string> g_WeaponListINI = { "Grip", "Shatter", "Spin", "Pierce", "Charge", "Surge" };

//===============
// Types
//===============
using _OnWeaponEquipped_Internal = void(*)(void* modelHandle, int slot, uint64_t weaponUID);
using _EquipNextWeapon_Internal = void(*)(void* gameInventoryComponentState);
using _OnGameEvent_Internal = void(*)(void* entityComponentState, char *event_message); // "4c 8b dc 57 48 81 ec a0 00 00 00 48 c7 44 24 20 fe ff ff ff 49 89 5b 18 48 8b 05"
using _Loadout_TypeRegistration = void(*)(void* a1, void* loadoutModelObject);
using _AppEventHandler = bool(*)(void* EventHandler_self, void* evt);
using _InputManager_IsMenuOn = bool(*)(void* inputManager);

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

using _ModelHandle_GetPropertyHandle = PropertyHandle * (*)(void* modelHandle, void* size_20h_struct, const char* propertyName);

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
static_assert(offsetof(GameWindowVtbl, GamepadButtonEvent) == 0x68, "offset error");
using _GameWindow_GetInstance = GameWindowVtbl**(*)();

struct GamepadButtonEvent {
    uint32_t unk00;
    uint32_t buttonID;
    bool isDown;
};

// Own types
struct Weapon {
    uint64_t uid;
    uint64_t gidEntity;
    const char* strName;
};

using WeaponSlot = std::pair<Weapon, int>;

enum WeaponSwitchMode {
    kSwitchMode_Inactive,   // Equip into the inactive slot
    kSwitchMode_Active,     // Equip into the active slot
    kSwitchMode_Secondary,  // Equip into the secondary slot
    kSwitchMode_Primary,    // Equip into the primary slot
};

//===================
// Addresses [7]
//===================
RVA<uintptr_t> Addr_Entry_GIDEntity_and_ModelHandle_GameInventoryComponentState_Offset("48 8B 83 ? ? ? ? 45 8B CC 48 8B 8E ? ? ? ? 48 89 84 24"); // Offset of the GIDEntity (first) and GameInventoryComponentState (second) from ModelHandle
RVA<uintptr_t> Addr_WeaponEntrySize("48 69 C0 ? ? ? ? 4C 63 E2 48 03 C7"); // Size of each weapon entry in the model
RVA<uintptr_t> Addr_GameInventoryComponentState_EquippedWeaponOffset("48 8B 89 ? ? ? ? 48 39 08 74 0C"); // Offset of currently equipped weapon from GameInventoryComponentState
RVA<_OnWeaponEquipped_Internal> OnWeaponEquipped_Internal("40 53 56 57 41 54 48 83 EC 48 48 8B B9"); // internal fn called when equipping weapons into loadout through UI
RVA<_EquipNextWeapon_Internal> EquipNextWeapon_Internal("40 53 48 83 EC 20 44 8B 49 68 48 8B D9"); // internal fn called when switching weapons via hotkey

// This happens when a game event takes place. We purposly avoid hooking
// an equip weapon function to avoid conflicts with other mods, like the
// weapon switching one
RVA<_OnGameEvent_Internal> OnGameEvent_Internal("4c 8b dc 57 48 81 ec a0 00 00 00 48 c7 44 24 20 fe ff ff ff 49 89 5b 18 48 8b 05");

RVA<uintptr_t> OnWeaponEquipped_SaveGame_Addr("33 D2 45 33 C0 8D 4A 02 FF 15 ? ? ? ? 4C 8D 05 ? ? ? ?", 8); // savegame call inside OnWeaponEquipped
_ModelHandle_GetPropertyHandle ModelHandle_GetPropertyHandle = nullptr; // from coherentuigt, to retrieve a property handle from a model handle by name
_GameWindow_GetInstance GameWindow_GetInstance = nullptr; // from coregame, to retrieve the GameWindow singleton instance
void** InputManager_ppInstance = nullptr; // from input - InputManager is created quite early, before startvideos.tex plays.
_InputManager_IsMenuOn InputManager_IsMenuOn = nullptr; // from input, to determine whether a menu (e.g. inventory / conversation / fast travel) is active

// Hooks
RVA<_Loadout_TypeRegistration> Loadout_TypeRegistration_Internal("E8 ? ? ? ? 48 8B D7 48 8B CB FF 15 ? ? ? ? 4D 8B 0E 48 8D 15 ? ? ? ?", 0, 1, 5); // hook this to receive loadoutModelHandle + 0x18 in a2
_Loadout_TypeRegistration Loadout_TypeRegistration_Original = nullptr;
//_OnWeaponEquipped_Internal OnWeaponEquipped_Original = nullptr;
//_EquipNextWeapon_Internal EquipNextWeapon_Original = nullptr;
_OnGameEvent_Internal OnGameEvent_Original = nullptr;
_AppEventHandler GameWindow_OnGamepadButtonEvent_Original = nullptr;


// Offsets / Struct Sizes
int g_WeaponEntry_GIDEntity_Offset = 0;
int g_ModelHandle_GameInventoryComponentState_Offset = 0;
int g_WeaponEntrySize = 0;
int g_GameInventoryComponentState_EquippedWeaponOffset = 0;

// Globals
ModelHandle* g_loadoutModelHandle = nullptr;

//===============================
// Utilities
//===============================

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

//=============
// Main
//=============

namespace DualsenseMod {

    // Read and populate offsets and addresses from game code
    bool PopulateOffsets() {
        g_WeaponEntry_GIDEntity_Offset = *reinterpret_cast<uint32_t*>(Addr_Entry_GIDEntity_and_ModelHandle_GameInventoryComponentState_Offset.GetUIntPtr() + 3);
        g_ModelHandle_GameInventoryComponentState_Offset = *reinterpret_cast<uint32_t*>(Addr_Entry_GIDEntity_and_ModelHandle_GameInventoryComponentState_Offset.GetUIntPtr() + 13);
        g_WeaponEntrySize = *reinterpret_cast<uint32_t*>(Addr_WeaponEntrySize.GetUIntPtr() + 3);
        g_GameInventoryComponentState_EquippedWeaponOffset = *reinterpret_cast<uint32_t*>(Addr_GameInventoryComponentState_EquippedWeaponOffset.GetUIntPtr() + 3);

        HMODULE hMod = GetModuleHandleA("coherentuigt.dll");
        ModelHandle_GetPropertyHandle = (_ModelHandle_GetPropertyHandle)GetProcAddress(hMod, "?GetPropertyHandle@ModelHandle@UIGT@Coherent@@QEBA?AUPropertyHandle@23@PEBD@Z");

        HMODULE hCoregame = GetRMDModule("coregame");
        GameWindow_GetInstance = (_GameWindow_GetInstance)GetProcAddress(hCoregame, "?getInstance@GameWindow@coregame@@SAPEAV12@XZ");

        HMODULE hInput = GetRMDModule("input");
        InputManager_ppInstance = (void**)GetProcAddress(hInput, "?sm_pInstance@InputManager@input@@0PEAV12@EA");
        InputManager_IsMenuOn = (_InputManager_IsMenuOn)GetProcAddress(hInput, "?isMenuOn@InputManager@input@@QEAA_NXZ");

        _LOG("Offsets: %X, %X, %X, %X", g_WeaponEntry_GIDEntity_Offset, g_ModelHandle_GameInventoryComponentState_Offset, g_WeaponEntrySize, g_GameInventoryComponentState_EquippedWeaponOffset);
        _LOG("OnWeaponEquipped_Internal at %p", OnWeaponEquipped_Internal.GetUIntPtr());
        _LOG("EquipNextWeapon_Internal at %p", EquipNextWeapon_Internal.GetUIntPtr());
        _LOG("OnGameEvent_Internal at %p", OnGameEvent_Internal.GetUIntPtr());
        _LOG("ModelHandle_GetPropertyHandle at %p", ModelHandle_GetPropertyHandle);
        _LOG("GameWindow_GetInstance at %p", GameWindow_GetInstance);
        _LOG("InputManager_pInstance at %p", InputManager_ppInstance);
        _LOG("InputManager_IsMenuOn at %p", InputManager_IsMenuOn);

        if (!ModelHandle_GetPropertyHandle || !GameWindow_GetInstance || !InputManager_ppInstance || !InputManager_IsMenuOn) return false;

        return true;
    }

    void Loadout_TypeRegistration_Hook(void* a1, void* loadoutModelObject) {
        if (!g_loadoutModelHandle) {
            g_loadoutModelHandle = (ModelHandle*)((char*)loadoutModelObject - 0x18);
            _LOG("g_loadoutModelHandle: %p", g_loadoutModelHandle);
        }
        Loadout_TypeRegistration_Original(a1, loadoutModelObject);
    }
#if 0
    void OnWeaponEquipped_Hook(void* modelHandle, int slot, uint64_t weaponUID) {
        _LOG("in OnWeaponEquipped hook! modelHandle: %p", modelHandle);
        OnWeaponEquipped_Original(modelHandle, slot, weaponUID);
    }

    void EquipNextWeapon_Hook(void* gameInventoryComponentState) {
        _LOG("in EquipNextWeapon hook!");
        EquipNextWeapon_Original(gameInventoryComponentState);
    }
#endif

    const char *getWeaponName (uint64_t gid);
    void OnGameEvent_Hook(void* entityComponentState, char *event_message) {
        OnGameEvent_Original(entityComponentState, event_message);
        if (strcmp(event_message, "weapon_equipped"))
            return;
        _LOG("in OnGameEvent hook! - event message: \"%s\"", event_message);

        if (!g_loadoutModelHandle) {
            _LOG("loadout not set yet, skipping...");
            return;
        }

        // Get current equipped weapon
        void* gameInventoryComponentState = Utils::GetOffset<void*>(g_loadoutModelHandle, g_ModelHandle_GameInventoryComponentState_Offset);
        uint64_t gidEquippedWeapon = Utils::GetOffset<uint64_t>(gameInventoryComponentState, g_GameInventoryComponentState_EquippedWeaponOffset);
        _LOG("* current weapon: (0x%x) - %u", gidEquippedWeapon, gidEquippedWeapon);
        const char* name = getWeaponName(gidEquippedWeapon);
        _LOG("* current weapon: %s", name);
    }

    void* getModelProperty(const char* propertyName) {
        if (!g_loadoutModelHandle) return nullptr;
        char a2[0x20];
        auto propertyHandle = ModelHandle_GetPropertyHandle(g_loadoutModelHandle, a2, propertyName);
        if (!propertyHandle) return nullptr;
        _LOG("getModelProperty: Computed offset for %s is 0x%x.", propertyName, propertyHandle->fieldProperty->offset);
        return (char*)&g_loadoutModelHandle->pDataStart + propertyHandle->fieldProperty->offset;
    }

    void Patch() {
        _LOG("Applying patches");
        // Disable the savegame call that happens when equipping weapons as this causes stutter and player may switch weapons often
        unsigned char data[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
        Utils::WriteMemory(OnWeaponEquipped_SaveGame_Addr.GetUIntPtr(), data, sizeof(data));
        _LOG("Patch done");
    }

    bool Hook() {
        _LOG("Applying hooks...");
        // Hook loadout type registration to obtain pointer to the model handle
        MH_Initialize();
        MH_CreateHook(Loadout_TypeRegistration_Internal, Loadout_TypeRegistration_Hook, reinterpret_cast<LPVOID*>(&Loadout_TypeRegistration_Original));
        if (MH_EnableHook(Loadout_TypeRegistration_Internal) != MH_OK) {
            _LOG("FATAL: Failed to install hook.");
            return false;
        }
#if 0
        MH_CreateHook(OnWeaponEquipped_Internal, OnWeaponEquipped_Hook, reinterpret_cast<LPVOID*>(&OnWeaponEquipped_Original));
        if (MH_EnableHook(OnWeaponEquipped_Internal) != MH_OK) {
            _LOG("FATAL: Failed to install OnWeaponEquipped_Internal hook.");
            return false;
        }

        MH_CreateHook(EquipNextWeapon_Internal, EquipNextWeapon_Hook, reinterpret_cast<LPVOID*>(&EquipNextWeapon_Original));
        if (MH_EnableHook(EquipNextWeapon_Internal) != MH_OK) {
            _LOG("FATAL: Failed to install EquipNextWeapon_Internal hook.");
            return false;
        }
#endif
        MH_CreateHook(OnGameEvent_Internal, OnGameEvent_Hook, reinterpret_cast<LPVOID*>(&OnGameEvent_Original));
        if (MH_EnableHook(OnGameEvent_Internal) != MH_OK) {
            _LOG("FATAL: Failed to install OnGameEvent_Internal hook.");
            return false;
        }
        _LOG("Hooks applied.");
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
                _LOG("Not resolved: %s", rvaData->sig);
            }
        }
        if (!RVAManager::IsAllResolved()) return false;

        return true;
    }

    // Checks whether the loadout model is valid. Must be checked before operating on the loadout model.
    bool isLoadoutModelValid() {
        if (!g_loadoutModelHandle) {
            _LOG("ERROR: Cannot equip any weapons when the loadout model handle is unset.");
            return false;
        }

        if (strcmp(g_loadoutModelHandle->modelName, "Loadout") != 0) {
            // Model name is no longer "Loadout" when the HUD is unloaded e.g. during load screens.
            _LOG("ERROR: Cannot equip any weapons when the loadout model is invalid.");
            return false;
        }

        return true;
    }

    // Operates on game type, wraps, extracts relevant data, and returns our own custom type.
    std::vector<Weapon> getWeapons(InPlaceVector* vecWeapons) {
        std::vector<Weapon> weapons;

        if (vecWeapons) {
            void* pItem = vecWeapons->items;
            _LOG("getWeapons() = vec size: %d", vecWeapons->size);
            for (unsigned int i = 0; i < vecWeapons->size; i++) {
                Weapon weapon = {};
                weapon.uid = Utils::GetOffset<uint64_t>(pItem, 0);
                weapon.gidEntity = Utils::GetOffset<uint64_t>(pItem, g_WeaponEntry_GIDEntity_Offset);
                weapon.strName = Utils::GetOffset<char*>(pItem, 0x10);
                _LOG("getWeapons() | weapon#%d - uid: %u, gidEntity: %u, name: %s", i+1, weapon.uid, weapon.gidEntity, weapon.strName);

                weapons.push_back(weapon);
                pItem = (char*)pItem + g_WeaponEntrySize;
            }
        }
        else {
            _LOG("WARNING: getWeapons(): vecWeapons is NULL.");
        }

        return weapons;
    }

    // Note: Loadout model must be valid before calling this function.
    std::vector<Weapon> getEquippedWeapons() {
        InPlaceVector* m_vecEquippedWeapons = (InPlaceVector*)getModelProperty("m_vecEquippedWeapons");
        return getWeapons(m_vecEquippedWeapons);
    }

    // TODO: add memoization for this one
    const char *getWeaponName (uint64_t gid) {
        std::vector<Weapon> weapons = getEquippedWeapons();
        for (auto weapon : weapons) {
            if (weapon.gidEntity == gid)
                return weapon.strName;
        }
        return NULL;
    }

    // Note: Loadout model must be valid before calling this function.
    std::vector<Weapon> getInventoryWeapons() {
        InPlaceVector* m_vecInventoryWeapons = (InPlaceVector*)getModelProperty("m_vecInventoryWeapons");
        return getWeapons(m_vecInventoryWeapons);
    }

    // Note: Loadout model must be valid before calling this function.
    // Obtains the weapon and slot index of the currently equipped weapon.
    // Note: Returns NULL if the currently equipped weapon cannot be found in the loadout model. Should never happen unless the loadout model or equipped weapon GID got desynced.
    std::shared_ptr<WeaponSlot> getCurrentlyEquippedWeapon(std::vector<Weapon>& equippedWeapons) {
        void* gameInventoryComponentState = Utils::GetOffset<void*>(g_loadoutModelHandle, g_ModelHandle_GameInventoryComponentState_Offset);
        uint64_t gidEquippedWeapon = Utils::GetOffset<uint64_t>(gameInventoryComponentState, g_GameInventoryComponentState_EquippedWeaponOffset);
        _LOG("* getCurrentlyEquippedWeapon: (0x%x) - %d", gidEquippedWeapon, gidEquippedWeapon);

        // Which slot is the currently equipped weapon in?
        auto equippedIt = std::find_if(equippedWeapons.begin(), equippedWeapons.end(), [&](const Weapon& weap) {
            return weap.gidEntity == gidEquippedWeapon;
        });

        if (equippedIt != equippedWeapons.end()) {
            int equippedWeaponSlot = static_cast<int>(equippedIt - equippedWeapons.begin());
            return std::make_shared<WeaponSlot>(*equippedIt, equippedWeaponSlot);
        }
        else {
            _LOG("ERROR: Could not find the currently equipped weapon (GID: %" PRIx64 ") in the equipped weapons list.");
            return nullptr;
        }
    }

    void equipWeapon(std::string weaponName, WeaponSwitchMode weaponSwitchMode) {
        if (!g_loadoutModelHandle) {
            _LOG("ERROR: Cannot equip any weapons when the loadout model handle is unset.");
            return;
        }

        if (strcmp(g_loadoutModelHandle->modelName, "Loadout") != 0) {
            // Model name is no longer "Loadout" when the HUD is unloaded e.g. during load screens.
            _LOG("ERROR: Cannot equip any weapons when the loadout model is invalid.");
            return;
        }

        _LOG("Weapon to equip: %s", weaponName.c_str());

        auto equippedWeapons = getEquippedWeapons();
        auto equippedWeapon = getCurrentlyEquippedWeapon(equippedWeapons);
        if (!equippedWeapon) return;

        int equippedWeaponSlot = equippedWeapon->second;
        int otherSlot = (equippedWeaponSlot + 1) % 2;

        // If the desired weapon is already equipped, nothing to do.
        if (strcmp(equippedWeapon->first.strName, weaponName.c_str()) == 0) {
            _LOG("Target weapon already equipped, nothing to do.");
            return;
        }

        // If the desired weapon is in the other equip slot, switch to it.
        if (strcmp(equippedWeapons[otherSlot].strName, weaponName.c_str()) == 0) {
            _LOG("Target weapon in other slot; switching to it.");
            void* gameInventoryComponentState = Utils::GetOffset<void*>(g_loadoutModelHandle, g_ModelHandle_GameInventoryComponentState_Offset);
            EquipNextWeapon_Internal(gameInventoryComponentState);
            return;
        }
        
        // Desired weapon is not equipped - we need to equip it.
        auto inventoryWeapons = getInventoryWeapons();
        auto weaponToEquipIt = std::find_if(inventoryWeapons.begin(), inventoryWeapons.end(), [&](const Weapon& weap) {
            return strcmp(weap.strName, weaponName.c_str()) == 0;
        });
        if (weaponToEquipIt == inventoryWeapons.end()) {
            _LOG("ERROR: Could not find the desired weapon (%s) in the weapon inventory.", weaponName.c_str());
            return;
        }

        // Equip the desired weapon in the desired equip slot. (Code automatically switches to it)
        int slotToEquipIn = otherSlot;
        switch (weaponSwitchMode) {
            case WeaponSwitchMode::kSwitchMode_Inactive:
                slotToEquipIn = otherSlot;
                break;
            case WeaponSwitchMode::kSwitchMode_Active:
                slotToEquipIn = equippedWeaponSlot;
                break;
            case WeaponSwitchMode::kSwitchMode_Primary:
                slotToEquipIn = 0;
                break;
            case WeaponSwitchMode::kSwitchMode_Secondary:
                slotToEquipIn = 1;
                break;
        }

        OnWeaponEquipped_Internal(g_loadoutModelHandle, slotToEquipIn, weaponToEquipIt->uid);

        _LOG("Done.");

    }

    // Note: returns empty string if the config doesn't exist or if the value is not one of the allowable values in g_WeaponListINI.
    // Check the .length() property to determine whether the returned value is valid or not.
    std::string getWeaponNameFromConfig(const char* sectionName, const char* keyName) {
        char szWeaponNameToActivate[255] = "";
        GetPrivateProfileStringA(sectionName, keyName, "<MISSING>", szWeaponNameToActivate, sizeof(szWeaponNameToActivate), INI_LOCATION);

        auto it = std::find(g_WeaponListINI.begin(), g_WeaponListINI.end(), szWeaponNameToActivate);
        if (it != g_WeaponListINI.end()) {
            auto idx = it - g_WeaponListINI.begin();
            return g_WeaponList[idx];
        }
        else {
            _LOG("WARNING: Invalid weapon '%s' used for %s::%s.", szWeaponNameToActivate, sectionName, keyName);
            return "";
        }
    }

    //=================
    // Input Handling
    //=================

    enum GamepadSwitchDirection {
        kGamepadSwitchDirection_Unset,
        kGamepadSwitchDirection_Left,
        kGamepadSwitchDirection_Right,
    };

    bool GameWindow_OnGamepadButtonEvent_Hook(void* self, GamepadButtonEvent* evt) {
        _LOG("GameWindow_OnGamepadButtonEvent: button: %d isDown: %d", evt->buttonID, evt->isDown);

        // Only interested in handling if not currently in a menu
        bool isMenuOn = InputManager_IsMenuOn(*InputManager_ppInstance);

        if (!isMenuOn && evt->isDown) {
            // 2 = D-Pad Left
            // 3 = D-Pad Right
            if (evt->buttonID == 2 || evt->buttonID == 3) {
                if (isLoadoutModelValid()) {
                    GamepadSwitchDirection switchDirection = evt->buttonID == 2 ? kGamepadSwitchDirection_Left : kGamepadSwitchDirection_Right;
                    std::vector<Weapon> inventoryWeapons = getInventoryWeapons();

                    // Build list of weapons for this direction from the config file.
                    // Only add weapons that the player has to the candidate weapon list.
                    std::vector<std::string> weapons;

                    for (int i = 1; i <= 3; i++) {
                        std::stringstream iniKeyName;
                        iniKeyName << "DPad" << (switchDirection == kGamepadSwitchDirection_Left ? "Left" : "Right") << i;
                        _LOG("Getting config from: %s", iniKeyName.str().c_str());

                        std::string weaponName = getWeaponNameFromConfig("GamepadConfig", iniKeyName.str().c_str());
                        if (weaponName.length()) {
                            // Add only weapons that the player possesses to the list.
                            auto it = std::find_if(inventoryWeapons.begin(), inventoryWeapons.end(), [&](const Weapon& weap) {
                                return strcmp(weap.strName, weaponName.c_str()) == 0;
                            });
                            if (it != inventoryWeapons.end()) {
                                weapons.push_back(weaponName);
                            }
                        }
                    }

                    if (weapons.size() > 0) {
                        // Try to locate the currently equipped weapon in the candidate list.
                        auto equippedWeapons = getEquippedWeapons();
                        auto equippedWeapon = getCurrentlyEquippedWeapon(equippedWeapons);
                        if (equippedWeapon) {
                            auto it = std::find_if(weapons.begin(), weapons.end(), [&](const std::string& weaponName) {
                                return strcmp(equippedWeapon->first.strName, weaponName.c_str()) == 0;
                            });

                            // If not found, or if last weapon, use first weapon as the next equip target.
                            if (it == weapons.end() || (it + 1) == weapons.end()) {
                                it = weapons.begin();
                            }
                            else {
                                // Else use next weapon in the candidate list
                                it++;
                            }

                            int weaponSwitchModeINI = GetPrivateProfileIntA("Settings", "GamepadSwitchMode", 2, INI_LOCATION);
                            WeaponSwitchMode weaponSwitchMode = kSwitchMode_Secondary;
                            switch (weaponSwitchModeINI) {
                                case 1:
                                    weaponSwitchMode = kSwitchMode_Inactive;
                                    break;
                                case 2:
                                    weaponSwitchMode = kSwitchMode_Secondary;
                                    break;
                                case 3:
                                    weaponSwitchMode = (switchDirection == kGamepadSwitchDirection_Left) ? kSwitchMode_Primary : kSwitchMode_Secondary;
                                    break;
                            }

                            equipWeapon(*it, weaponSwitchMode);
                        }
                    }
                    else {
                        _LOG("WARNING: Player has no valid weapons to switch to!");
                    }
                }
            }
        }
        return GameWindow_OnGamepadButtonEvent_Original(self, evt);
    }

    WNDPROC WndProc_Original;
    LRESULT CALLBACK WndProc_Hook(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        // Special case: F10 sends WM_SYSKEYUP
        if (uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP) {
            // 1-6 keys
            if (wParam >= 0x31 && wParam <= 0x36) {
                // Only interested in handling if not currently in a menu
                bool isMenuOn = InputManager_IsMenuOn(*InputManager_ppInstance);
                if (!isMenuOn) {
                    int key = (int)wParam - 0x30;
                    _LOG("Hotkey %d pressed.", key);

                    std::stringstream iniKeyName;
                    iniKeyName << "Weapon" << key;

                    std::string weaponName = getWeaponNameFromConfig("KeyboardConfig", iniKeyName.str().c_str());

                    if (weaponName.length()) {
                        int weaponSwitchModeINI = GetPrivateProfileIntA("Settings", "KBSwitchMode", 1, INI_LOCATION);
                        WeaponSwitchMode weaponSwitchMode = kSwitchMode_Inactive;
                        switch (weaponSwitchModeINI) {
                            case 1:
                                weaponSwitchMode = kSwitchMode_Inactive;
                                break;
                            case 2:
                                weaponSwitchMode = kSwitchMode_Secondary;
                                break;
                        }

                        equipWeapon(weaponName, weaponSwitchMode);
                    }
                }            
            }
        }

        return CallWindowProc(WndProc_Original, hwnd, uMsg, wParam, lParam);
    }

    void Init() {
#if 0
        char logPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, NULL, logPath))) {
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
            MessageBoxA(NULL, "DualsenseMod is not compatible with this version of Control.\nPlease visit the mod page for updates.", "DualsenseMod", MB_OK | MB_ICONEXCLAMATION);
            _LOG("FATAL: Incompatible version");
            return;
        }
        _LOG("Addresses set");

        Patch();
        Hook();
        
        auto hookThread = [](LPVOID data) -> DWORD {
            int numAttempts = 10;
            while (numAttempts--) {
                _LOG("Waiting to get window...");
                Sleep(2000); // rudimentary wait for window to be created
                HWND hwnd = Utils::FindOwnWindow();
                if (hwnd) {
                    _LOG("Found Control window.");
                    // Hook WndProc
                    WndProc_Original = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc_Hook);

                    // VFT hook for GameWindow::handleAppEvent(app::GamepadButtonEvent*)
                    GameWindowVtbl** pGameWindowVtbl = GameWindow_GetInstance();
                    if (!pGameWindowVtbl) {
                        _LOG("Failed to obtain game window instance.");
                        return 0;
                    }
                    GameWindowVtbl* gameWindowVtbl = *pGameWindowVtbl;
                    _LOG("GameWindow at %p, vtbl at %p", pGameWindowVtbl, gameWindowVtbl);
                    GameWindow_OnGamepadButtonEvent_Original = (_AppEventHandler)Utils::VFTHook(&gameWindowVtbl->GamepadButtonEvent, GameWindow_OnGamepadButtonEvent_Hook);

                    _LOG("Completed window actions.");

                    return 0;
                }
            }
            _LOG("FATAL: Failed to find Control window.");
            MessageBoxA(NULL, "Failed to find Control", "Warning", MB_OK | MB_ICONEXCLAMATION);
            return 0;
        };
        CreateThread(NULL, 0, hookThread, NULL, 0, NULL);

        _LOG("Ready.");
    }
}
