#include "imports.h"
#include "globals.h"
#include "misc/security.h"

#include "hooks.hpp"
HINSTANCE DllHandle;

#define WIN32_LEAN_AND_MEAN

DWORD WINAPI InitExec() {
    std::cout << _XOR_("[-] MainThread +\n");
    if (MH_Initialize() != MH_OK) {
        return 0;
    }
    std::cout << _XOR_("[HOOKLIB] +\n");

    if (MH_CreateHookApi(L"kernel32.dll", _XOR_("GetThreadContext"), &hookedGetThreadContext, reinterpret_cast<void**>(&pGetThreadContext)) != MH_OK) {
        MessageBoxA(NULL, _XOR_("Failed To Hook GetThreadContext"), "", MB_OK);
    }
    else {
        std::cout << _XOR_("[security-enabled] GetThreadContext->dr. for HW Breakpoints\n");
    }

    if (MH_CreateHookApi(L"ntdll.dll", _XOR_("NtRaiseHardError"), &hookedNtRaiseHardError, reinterpret_cast<void**>(&pNtRaiseHardError)) != MH_OK) {
        MessageBoxA(NULL, _XOR_("Failed To Hook NtRaisedHardError"), "", MB_OK);
    }
    else {
        std::cout << _XOR_("[security-enabled] GetThreadContext->dr. for HW Breakpoints\n");
    }

    if (MH_CreateHookApi(L"kernel32.dll", _XOR_("WriteProcessMemory"), &hookedWriteProcessMemory, reinterpret_cast<void**>(&pWriteProcessMemory)) != MH_OK) {
        MessageBoxA(NULL, _XOR_("Failed To Hook WriteProcessMemory"), "", MB_OK);
    }
    else {
        std::cout << _XOR_("[enabled] WriteProcessMemory Dumper\n");
    }

    if (MH_CreateHookApi(L"kernel32.dll", _XOR_("ReadProcessMemory"), &hookedReadProcessMemory, reinterpret_cast<void**>(&pReadProcessMemory)) != MH_OK) {
        MessageBoxA(NULL, _XOR_("Failed To Hook ReadProcessMemory"), "", MB_OK);
    }
    else {
        std::cout << _XOR_("[enabled] ReadProcessMemory Dumper\n");
    }

    if (MH_CreateHookApi(L"kernel32.dll", _XOR_("DeleteFileW"), &hookedDeleteFileW, reinterpret_cast<void**>(&pDeleteFileW)) != MH_OK) {
        MessageBoxA(NULL, _XOR_("Failed To Hook DeleteFileW"), "", MB_OK);
    }
    else {
        std::cout << _XOR_("[enabled] DeleteFileW Dumper\n");
    }

    if (MH_CreateHookApi(L"kernel32.dll", _XOR_("DeleteFileA"), &hookedDeleteFileA, reinterpret_cast<void**>(&pDeleteFileA)) != MH_OK) {
        MessageBoxA(NULL, _XOR_("Failed To Hook DeleteFileA"), "", MB_OK);
    }
    else {
        std::cout << _XOR_("[enabled] DeleteFileA Dumper\n");
    }



    MH_EnableHook(MH_ALL_HOOKS);
    while (true) {
        Sleep(50);
        if (GetAsyncKeyState(VK_END) & 1) {
            break;
        }
    }

    return 0;
}

int __stdcall DllMain(const HMODULE hModule, const std::uintptr_t reason, const void* reserved) {
    if (reason == 1) {
        if (globals::AllocateConsole == true) {
            AllocConsole();
            FILE* fp;
            freopen_s(&fp, "CONOUT$", "w", stdout);
        }

        DisableThreadLibraryCalls(hModule);
        std::cout << _XOR_("[-] AllocConsole - freopen_s | SET\n");
        DllHandle = hModule;

        hyde::CreateThread(InitExec, DllHandle);
        std::cout << _XOR_("[-] Started Main Thread...\n");

        return true;
    }
    return true;
}

