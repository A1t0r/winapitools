#ifndef WINAPITOOLS
#define WINAPITOOLS

#include <windows.h>
#include <vector>
#include <string>
#include <iostream>
#include <cstdint>
#include <stdexcept>

namespace WinAPITools{
    struct handleData {
        DWORD processId;
        HWND hWnd;
        char* title;
        char* className;
    };

    struct windowsVersion {
        DWORD major;
        DWORD minor;
    };

    // struct windowData {
    //     HWND hWnd;
    //     HWND parentHWnd;
    //     std::string title;
    //     std::string class;
    // };
    HANDLE getProcessByName(LPCSTR processName);
    DWORD getProcessIdByName(LPCSTR processName);
    DWORD getModuleBaseAddress(const DWORD processID, LPCSTR moduleName);
    HANDLE getProcessByID(DWORD processID);
    
    void writeBytes(HANDLE hProcess, void* addr, const std::vector<BYTE>& bytes);
    void writeFloat(HANDLE hProcess, void* addr);
    void writeDouble(HANDLE hProcess, void* addr);
    std::string readString(HANDLE hProcess, void* addr);
    HWND getMainWindow(DWORD pId);
    BOOL CALLBACK callbackByTitle(HWND handle, LPARAM lParam);
    BOOL CALLBACK enumWindowsCallback(HWND handle, LPARAM lParam);
    BOOL isMainWindow(HWND handle);
    HWND getWindowByTitle(const char* title);
    void keyPress(const HWND hWnd, const uint8_t key);
    void keyHold(const HWND hWnd, const uint8_t key, bool down);
    void kbHotkeyPress(const HWND hWnd, const DWORD modifier, const DWORD key);
    // BITMAP takeScreenshot(HWND hWnd);
    windowsVersion getWindowsVersion();
    BOOL CALLBACK callbackTitles(HWND handle, LPARAM lParam);
    std::vector<std::string> getChildTitles(HWND hWnd);
    BOOL CALLBACK callbackClasses(HWND handle, LPARAM lParam);
    std::vector<std::string> getChildClasses(HWND hWnd);
    BOOL CALLBACK callbackClass(HWND handle, LPARAM lParam);
    HWND getWindowByClass(HWND hWnd, const char* className);
    HWND getWindowByPos(const int x, const int y);
    BOOL moveMouseTo(const int x, const int y, HWND hWnd = NULL);
    BOOL clickMouse(const int x, const int y);
    BOOL clickMouseClient(HWND hWnd, const int x, const int y);

    BOOL hwClickMouse(const int x, const int y);
    BOOL hwClickMouseClient(HWND hWnd, const int x, const int y);
    BOOL hwMoveMouse(const int x, const int y);
    BOOL hwMoveMouseClient(HWND hWnd, const int x, const int y);
    BOOL hwKeypress(const std::string& str);
    BOOL hwVKKeypress(const BYTE vk);
    BOOL hwDblClickMouse(const int x, const int y);
    void printChildWindows(const HWND parentHWnd);

    SIZE getWindowSize(const HWND hWnd);
    

    template<typename T>
    void writeToMemory(HANDLE hProcess, void* addr, T value)
    {
        DWORD _err;
        DWORD oldprotect;

        VirtualProtectEx(hProcess, addr, sizeof(value), PAGE_EXECUTE_READWRITE, &oldprotect);
        if (WriteProcessMemory(hProcess, addr, &value, sizeof(value), NULL) != 0){
            std::cout << "Successfully wrote " << value << " at address " << addr << std::endl;
            VirtualProtectEx(hProcess, addr, sizeof(value), oldprotect, &oldprotect);
        } else {
            VirtualProtectEx(hProcess, addr, sizeof(value), oldprotect, &oldprotect);
            _err = GetLastError();
            std::cout << "unable to write to memory. Failed " << _err << std::endl;
            throw std::runtime_error({ "unable to write to memory. Failed" });
        }
    }



    template<typename T>
    T readValue(HANDLE hProcess, void* addr)
    {
        DWORD _err;
        T value { 0 };
        if (ReadProcessMemory(hProcess, addr, &value, sizeof(T), NULL) == 0){
            _err = GetLastError();
            std::cout << "unable to read from memory. Failed " << _err << std::endl;
            throw std::runtime_error({ "unable to read from memory. Failed" });
        }
        return value;
    }



    template<typename T>
    void writeToMemoryByPointers(HANDLE hProcess, const DWORD addr, T value, const std::vector<DWORD>& offsets = {})
    {
        auto address = readValue<DWORD>(hProcess, reinterpret_cast<void*>(static_cast<size_t>(addr)));
        for (auto it = offsets.cbegin(); it < offsets.cend(); ++it){
            address += *it;
            if (it != offsets.cend() - 1)
                address = readValue<DWORD>(hProcess, reinterpret_cast<void*>(static_cast<size_t>(address)));
        }
        writeToMemory<T>(hProcess, reinterpret_cast<void*>(static_cast<size_t>(address)), value);
    }
}

#endif