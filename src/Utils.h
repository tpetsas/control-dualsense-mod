#pragma once
#include <cstdint>

#ifndef _WINDEF_
struct HWND__;
typedef HWND__* HWND;
#endif

// reg2k
namespace Utils {
    bool ReadMemory(uintptr_t addr, void* data, size_t len);
    void WriteMemory(uintptr_t addr, void* data, size_t len);
    void* VFTHook(void* addr, void* newFunc);
    uintptr_t GetRelative(uintptr_t start, int offset = 1, int instructionLength = 5);

    template <typename T>
    T GetVirtualFunction(void* baseObject, int vtblIndex) {
        uintptr_t* vtbl = reinterpret_cast<uintptr_t**>(baseObject)[0];
        return reinterpret_cast<T>(vtbl[vtblIndex]);
    }

    template <typename T>
    T GetOffset(const void* baseObject, int offset) {
        return *reinterpret_cast<T*>((uintptr_t)baseObject + offset);
    }

    template<typename T>
    T* GetOffsetPtr(const void * baseObject, int offset) {
        return reinterpret_cast<T*>((uintptr_t)baseObject + offset);
    }

    uint64_t GetGameVersion();

    HWND FindOwnWindow();
}