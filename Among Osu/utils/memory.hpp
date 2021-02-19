//
// Created by he4rt on 2021-02-19.
//

#ifndef AMONG_OSU_MEMORY_HPP
#define AMONG_OSU_MEMORY_HPP

#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <cstdint>
#include <string.h>
#include <iostream>

namespace memory {
    static HANDLE h_process;
    static uintptr_t game_base;

    template <typename t>
    t read(const uintptr_t address)
    {
        t buffer{};
        ReadProcessMemory(h_process, reinterpret_cast<LPVOID>(address), &buffer, sizeof(buffer), nullptr);
        return buffer;
    }

    template <typename t>
    bool read(const uintptr_t address, t buffer, const size_t size)
    {
        return ReadProcessMemory(h_process, reinterpret_cast<LPVOID>(address), buffer, size, nullptr);
    }

    template <typename t>
    bool write(const uintptr_t address, t buffer)
    {
        return WriteProcessMemory(h_process, reinterpret_cast<LPVOID>(address), &buffer, sizeof(buffer), nullptr);
    }

    template <typename t>
    bool write(const uintptr_t address, t buffer, const size_t size)
    {
        return WriteProcessMemory(h_process, reinterpret_cast<LPVOID>(address), reinterpret_cast<LPCVOID>(buffer), size, nullptr);
    }

    inline uintptr_t get_module_base_address(const LPCWSTR mod_name)
    {
        DWORD cb = 0;
        HMODULE mods[512] = {};
        EnumProcessModulesEx(h_process, mods, sizeof(mods), &cb, LIST_MODULES_ALL);
        for (SIZE_T i = 0; i < cb / sizeof(HMODULE); i++)
        {
            if (mods[i])
            {
                wchar_t baseName[MAX_PATH] = {};
                GetModuleBaseNameW(h_process, mods[i], baseName, MAX_PATH);

                MODULEINFO mod = {};
                GetModuleInformation(h_process, mods[i], &mod, sizeof(mod));

                if (!_wcsicmp(baseName, mod_name) && mod.lpBaseOfDll != GetModuleHandle(nullptr))
                    return reinterpret_cast<uintptr_t>(mod.lpBaseOfDll);
            }
        }

        return NULL;
    }

    inline uint32_t get_pid_by_name(const std::wstring &process_name)
    {
        PROCESSENTRY32 entry{};
        entry.dwSize = sizeof(PROCESSENTRY32);

        auto *const snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

        if (Process32First(snapshot, &entry) == TRUE)
        {
            while (Process32Next(snapshot, &entry) == TRUE)
            {
                if (!_wcsicmp(entry.szExeFile, process_name.c_str()) && entry.th32ProcessID != GetCurrentProcessId())
                {
                    auto *const h_process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);

                    if (h_process)
                    {
                        CloseHandle(h_process);
                        return entry.th32ProcessID;
                    }
                }
            }
        }

        CloseHandle(snapshot);

        return NULL;
    }

    inline void enable_debug_privilege()
    {
        HANDLE h_token{};
        TOKEN_PRIVILEGES token_privileges{};
        LUID uid_debug;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &h_token) != FALSE)
        {
            if (LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &uid_debug) != FALSE)
            {
                token_privileges.PrivilegeCount = 1;
                token_privileges.Privileges[0].Luid = uid_debug;
                token_privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
                AdjustTokenPrivileges(h_token, FALSE, &token_privileges, 0, nullptr, nullptr);
            }
        }
        CloseHandle(h_token);
    }

    inline void initialize()
    {
        h_process = OpenProcess(PROCESS_ALL_ACCESS, false, get_pid_by_name(L"osu!.exe"));
        game_base = get_module_base_address(L"osu!.exe");
        std::cout << "[+] game_base: " << std::hex << game_base << std::dec << std::endl;
    }
}

using namespace memory;

#endif //AMONG_OSU_MEMORY_HPP
