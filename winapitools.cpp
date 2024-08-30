#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include "winapitools.h"
#include <stdexcept>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

HANDLE WinAPITools::getProcessByName(LPCSTR processName)
{
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapProcess = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapProcess == INVALID_HANDLE_VALUE){
        CloseHandle(snapProcess);
        throw std::runtime_error("Snapshot failed! " + GetLastError());
    }
    if (Process32First(snapProcess, &pe) == FALSE){
        CloseHandle(snapProcess);
        throw std::runtime_error("First process failed!" + GetLastError());
    }
    do{
        if (strstr(static_cast<char*>(pe.szExeFile), processName) != NULL){
            CloseHandle(snapProcess);
            HANDLE hProcess = getProcessByID(pe.th32ProcessID);
            return hProcess;
        }
    }while(Process32Next(snapProcess, &pe));
    CloseHandle(snapProcess);
    throw std::runtime_error("Process not found!");
}



DWORD WinAPITools::getProcessIdByName(LPCSTR processName)
{
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapProcess = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapProcess == INVALID_HANDLE_VALUE){
        CloseHandle(snapProcess);
        throw std::runtime_error("Snapshot failed! " + GetLastError());
    }
    if (Process32First(snapProcess, &pe) == FALSE){
        CloseHandle(snapProcess);
        throw std::runtime_error("First process failed!" + GetLastError());
    }
    do{
        if (strstr(static_cast<char*>(pe.szExeFile), processName) != NULL){
            CloseHandle(snapProcess);
            
            return pe.th32ProcessID;
        }
    }while(Process32Next(snapProcess, &pe));
    CloseHandle(snapProcess);
    throw std::runtime_error("Process not found!");
}



DWORD WinAPITools::getModuleBaseAddress(const DWORD processID, LPCSTR moduleName)
{
    MODULEENTRY32 me;
    me.dwSize = sizeof(MODULEENTRY32);
    HANDLE snapModule = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processID);
    if (snapModule == INVALID_HANDLE_VALUE){
        CloseHandle(snapModule);
        throw std::runtime_error("Snapshot failed! " + GetLastError());
    }
    if (Module32First(snapModule, &me) == FALSE){
        CloseHandle(snapModule);
        throw std::runtime_error("First module failed!" + GetLastError());
    }
    do{
        if (strstr(me.szModule, moduleName) != NULL){
            CloseHandle(snapModule);
            return reinterpret_cast<DWORD&>(me.modBaseAddr);
        }
    }while(Module32Next(snapModule, &me));
    CloseHandle(snapModule);
    throw std::runtime_error("Module not found!");
}

HANDLE WinAPITools::getProcessByID(DWORD processID)
{
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if ( hProcess != NULL){
        return hProcess;
    } else {
        throw std::runtime_error("Error opening process!");
    }
}

void WinAPITools::writeBytes(HANDLE hProcess, void* addr, const std::vector<BYTE>& bytes)
{
    DWORD _err;
    if (WriteProcessMemory(hProcess, addr, &bytes[0], bytes.size(), NULL) != 0){
        std::cout << "Successfully inject code at address " << addr << std::endl;
    } else {
        _err = GetLastError();
        std::cout << "unable to write to memory. Failed " << _err << std::endl;
    }
}



void WinAPITools::writeFloat(HANDLE hProcess, void* addr)
{
    DWORD _err;
    FLOAT value;
    std::cout << "Enter the float value to write: ";
    std::cin >> value;
    if (WriteProcessMemory(hProcess, addr, &value, sizeof(value), NULL) != 0){
        std::cout << "Successfully wrote " << value << " at address " << addr << std::endl;
    } else {
        _err = GetLastError();
        std::cout << "unable to write to memory. Failed " << _err << std::endl;
    }
}

void WinAPITools::writeDouble(HANDLE hProcess, void* addr)
{
    DWORD _err;
    DOUBLE value;
    std::cout << "Enter the double value to write: ";
    std::cin >> value;
    if (WriteProcessMemory(hProcess, addr, &value, sizeof(value), NULL) != 0){
        std::cout << "Successfully wrote " << value << " at address " << addr << std::endl;
    } else {
        _err = GetLastError();
        std::cout << "unable to write to memory. Failed " << _err << std::endl;
    }
}

std::string WinAPITools::readString(HANDLE hProcess, void* addr)
{
    DWORD _err;
    char c;
    std::string result { "" };
    do{
        if (ReadProcessMemory(hProcess, addr, &c, 1, NULL) == 0){
            _err = GetLastError();
            std::cout << "unable to read from memory. Failed " << _err << std::endl;
        }
        result += c;
        addr = reinterpret_cast<void*>(reinterpret_cast<size_t>(addr) + 1);
    }while (static_cast<int>(c) != 0 );
    return result;
}



BOOL WinAPITools::isMainWindow(HWND handle)
{
    return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
}



BOOL CALLBACK WinAPITools::enumWindowsCallback(HWND handle, LPARAM lParam)
{
    handleData& data = *(handleData*)lParam;
    DWORD pId{ 0 };

    GetWindowThreadProcessId(handle, &pId);
    if (data.processId != pId || !isMainWindow(handle)){
        return TRUE;
    }
    data.hWnd = handle;
    return FALSE;
}


HWND WinAPITools::getMainWindow(DWORD pId)
{   
    handleData data;
    data.processId = pId;
    data.hWnd = 0;
    EnumWindows(enumWindowsCallback, (LPARAM)&data);
    return data.hWnd;
}


BOOL CALLBACK WinAPITools::callbackByTitle(HWND handle, LPARAM lParam)
{
    handleData& data = *((handleData*)lParam);
    char title[255];
    SendMessage(handle, WM_GETTEXT, 255, reinterpret_cast<LPARAM>(title));
    if (!isMainWindow(handle) || strstr(title, data.title) == NULL){
        return TRUE;
    }
    data.hWnd = handle;
    return FALSE;
}


HWND WinAPITools::getWindowByTitle(const char* title)
{   
    handleData data;
    data.processId = 0;
    data.hWnd = NULL;
    data.title = const_cast<char*>(title);
    EnumWindows(callbackByTitle, (LPARAM)&data);
    return data.hWnd;
}


BOOL CALLBACK WinAPITools::callbackClass(HWND handle, LPARAM lParam)
{
    handleData& data = *((handleData*)lParam);
    char wndClass[255];
    GetClassName(handle, wndClass, 255);
    if (strstr(wndClass, data.className) != NULL){
        data.hWnd = handle;
        return FALSE;
    }
    return TRUE;
    
}


HWND WinAPITools::getWindowByClass(HWND hWnd, const char* className)
{   
    handleData data{
        .hWnd { 0 },
        .className { const_cast<char*>(className) }
    };
    EnumChildWindows(hWnd, callbackClass, (LPARAM)&data);
    return data.hWnd;
}


void WinAPITools::keyHold(const HWND hWnd, const uint8_t key, bool down)
{
    if (down)
        SendMessage(hWnd, WM_KEYDOWN, key, 0);
    else
        SendMessage(hWnd, WM_KEYUP, key, 0);
}


void WinAPITools::keyPress(const HWND hWnd, const uint8_t key)
{
    SendMessage(hWnd, WM_KEYDOWN, key, 0);
    SendMessage(hWnd, WM_KEYUP, key, 0);
}


WinAPITools::windowsVersion WinAPITools::getWindowsVersion()
{
    DWORD dwVersion{ 0 };
    dwVersion = GetVersion();
    WinAPITools::windowsVersion version{
        .major = static_cast<DWORD>(LOBYTE(LOWORD(dwVersion))),
        .minor = static_cast<DWORD>(HIBYTE(LOWORD(dwVersion)))
    };
    
    return version;
}


BOOL CALLBACK WinAPITools::callbackTitles(HWND handle, LPARAM lParam)
{
    std::vector<std::string>& titles = *((std::vector<std::string>*)lParam);
    char title[255];
    SendMessage(handle, WM_GETTEXT, 255, reinterpret_cast<LPARAM>(title));
    titles.push_back(std::string().assign(title));
    return TRUE;
    
}


std::vector<std::string> WinAPITools::getChildTitles(HWND hWnd)
{
    std::vector<std::string> titles;
    EnumChildWindows(hWnd, callbackTitles, (LPARAM)&titles);
    return titles;
}


BOOL CALLBACK WinAPITools::callbackClasses(HWND handle, LPARAM lParam)
{
    std::vector<std::string>& classes = *((std::vector<std::string>*)lParam);
    char wndClass[255];
    GetClassName(handle, wndClass, 255);
        // return FALSE;
    classes.push_back(std::string().assign(wndClass));
    return TRUE;
    
}


std::vector<std::string> WinAPITools::getChildClasses(HWND hWnd)
{
    std::vector<std::string> classes;
    EnumChildWindows(hWnd, callbackClasses, (LPARAM)&classes);
    return classes;
}


HWND WinAPITools::getWindowByPos(const int x, const int y)
{
    POINT p{
        .x = x,
        .y = y
    };
    return WindowFromPoint(p);
}

BOOL WinAPITools::moveMouseTo(const int x, const int y, HWND hWnd)
{
    if (hWnd == NULL)
        hWnd = WinAPITools::getWindowByPos(x, y);
    SendMessageA(hWnd, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(x, y));
    return TRUE;
}


BOOL WinAPITools::hwClickMouseClient(HWND hWnd, const int x, const int y)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    POINT p{
		.x = x,
		.y = y
	};
	ClientToScreen(hWnd, &p);
    return hwMoveMouse(p.x, p.y) && hwClickMouse(p.x, p.y);
}



BOOL WinAPITools::hwMoveMouseClient(HWND hWnd, const int x, const int y)
{
    POINT p{
		.x = x,
		.y = y
	};
	ClientToScreen(hWnd, &p);
    return hwMoveMouse(p.x, p.y);
}


BOOL WinAPITools::hwClickMouse(const int x, const int y)
{
    INPUT in[2];
    MOUSEINPUT mi1{
        .dx = (static_cast<LONG>(x) + 8) * (65536 / GetSystemMetrics(SM_CXSCREEN)),
        .dy = (static_cast<LONG>(y) + 8) * (65536 / GetSystemMetrics(SM_CYSCREEN)),
        .dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN,
        .time = 0
    };
    in[0] = {
        .type = INPUT_MOUSE,
        .mi = mi1
    }; 
    MOUSEINPUT mi2{
        .dx = (static_cast<LONG>(x) + 8) * (65536 / GetSystemMetrics(SM_CXSCREEN)),
        .dy = (static_cast<LONG>(y) + 8) * (65536 / GetSystemMetrics(SM_CYSCREEN)),
        .dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP,
        .time = 0
    };
    in[1] = {
        .type = INPUT_MOUSE,
        .mi = mi2
    }; 
    return (SendInput(2, in, sizeof(INPUT)) == 2);
}

BOOL WinAPITools::hwMoveMouse(const int x, const int y)
{
    INPUT in[1];
    MOUSEINPUT mi{
        .dx = (static_cast<LONG>(x) + 8) * (65536 / GetSystemMetrics(SM_CXSCREEN)),
        .dy = (static_cast<LONG>(y) + 8) * (65536 / GetSystemMetrics(SM_CYSCREEN)),
        .dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE,
        .time = 0
    };
    in[0] = {
        .type = INPUT_MOUSE,
        .mi = mi
    }; 
    return (SendInput(1, in, sizeof(INPUT)) == 1);
}


BOOL WinAPITools::hwKeypress(const std::string& str)
{
    for(const auto& s: str){
        INPUT input;
        input.type = INPUT_KEYBOARD;
        input.ki.wScan = s;
        input.ki.wVk = 0;
        input.ki.dwFlags = KEYEVENTF_UNICODE;
        if (SendInput(1, &input, sizeof(INPUT)) != 1)
            return FALSE;
    }
    return TRUE;
}


BOOL WinAPITools::hwVKKeypress(const BYTE vk)
{
    
    INPUT input;
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = 0;
    input.ki.wVk = vk;
    input.ki.dwFlags = 0;
    if (SendInput(1, &input, sizeof(INPUT)) != 1)
        return FALSE;
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    return SendInput(1, &input, sizeof(INPUT)) == 1;
}


BOOL WinAPITools::hwDblClickMouse(const int x, const int y)
{
    INPUT in[1];
    MOUSEINPUT mi{
        .dx = (static_cast<LONG>(x) + 8) * (65536 / GetSystemMetrics(SM_CXSCREEN) - 1),
        .dy = (static_cast<LONG>(y) + 8) * (65536 / GetSystemMetrics(SM_CYSCREEN) - 1),
        .dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP,
        .time = 0
    };
    in[0] = {
        .type = INPUT_MOUSE,
        .mi = mi
    }; 
    return hwClickMouse(x, y) && (SendInput(1, in, sizeof(INPUT)) == 1) && (SendInput(1, in, sizeof(INPUT)) == 1);
}


BOOL enumWindowsCallbackPrint(HWND handle, LPARAM lParam)
{
    size_t& nest = *((size_t*)lParam);
    char title[255];
    char wndClass[255];
    // SendMessage(handle, WM_GETTEXT, 255, reinterpret_cast<LPARAM>(title));
    GetWindowText(handle, title, 255);

    GetClassName(handle, wndClass, 255);
    std::cout << std::string(nest, '\t') << title << " | " << wndClass << std::endl;
    // HMENU hMenu = GetMenu(handle);
    // if (hMenu != NULL){
    //     std::cout << std::string(nest, '\t') << "Menu found! " << hMenu << " Count: " << GetMenuItemCount(hMenu) << std::endl;
        // std::cout << GetLastError() << std::endl;
        // std::cout << GetMenuItemID(hMenu, 0) << std::endl;
        // char lpString[255];
        // GetMenuString(hMenu, 0, lpString, 255, 0);
        // std::cout << lpString << std::endl;
        // UINT itemID = GetMenuItemID(hMenu, 0);
        // std::cout << "ItemID: " << itemID << std::endl;
    // }

        
    ++nest;
    EnumChildWindows(handle, enumWindowsCallbackPrint, (LPARAM)&nest);
    --nest;
    return TRUE;
}

void WinAPITools::printChildWindows(const HWND parentHWnd)
{   
    size_t nest = 0;
    if (parentHWnd != NULL){
        EnumChildWindows(parentHWnd, enumWindowsCallbackPrint, (LPARAM)&nest);
    } else {
        
        EnumWindows(enumWindowsCallbackPrint, (LPARAM)&nest);
        
    }
}


BOOL WinAPITools::clickMouse(const int x, const int y)
{
    HWND hWnd { WinAPITools::getWindowByPos(x, y)};
    POINT p{
        p.x = x,
        p.y = y
    };
    ScreenToClient(hWnd, &p);
    clickMouseClient(hWnd, p.x, p.y);
    return TRUE;
}


BOOL WinAPITools::clickMouseClient(HWND hWnd, const int x, const int y)
{
    // std::cout << "click on: " << hWnd << "  " << x << ", " << y << std::endl;
    SendMessage(hWnd, WM_LBUTTONDOWN, 0, MAKELPARAM(x, y));
    //PostMessage(hWnd, WM_LBUTTONDOWN, 0, MAKELPARAM(x, y));
    //std::this_thread::sleep_for(std::chrono::milliseconds(200));
    SendMessage(hWnd, WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(x, y));
    //PostMessage(hWnd, WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(x, y));
    
    return TRUE;
}


SIZE WinAPITools::getWindowSize(const HWND hWnd)
{
    RECT r;
    GetWindowRect(hWnd, &r);
    SIZE size { 
        r.right - r.left, 
        r.bottom - r.top
    };
    return size;
}


void WinAPITools::kbHotkeyPress(const HWND hWnd, const DWORD modifier, const DWORD key)
{
    DWORD currentThreadId{ 0 }, remoteThreadId{ 0 };
	currentThreadId = GetCurrentThreadId();
	std::cout << std::hex << currentThreadId << std::endl;
	remoteThreadId = GetWindowThreadProcessId(hWnd, nullptr);
	std::cout << std::hex << GetWindowThreadProcessId(hWnd, nullptr) << std::endl;
	AttachThreadInput(currentThreadId, remoteThreadId, true);
	SetFocus(hWnd);
	BYTE keys[256];
	GetKeyboardState(keys);
	keys[modifier] = 0x80;
	SetKeyboardState(keys);
	
	SendMessage(hWnd, WM_KEYDOWN, key, 0);

	keys[modifier] = 0x00;
	SetKeyboardState(keys);
	AttachThreadInput(currentThreadId, remoteThreadId, false);
}

// BITMAP WinAPITools::takeScreenshot(HWND hWnd = NULL)
// {
//     HWND hWnd = hWnd ? hWnd : GetDesktopWindow();
//     HDC hdc = GetDC(hWnd);
//     HDC hdcCompatible = CreateCompatibleDC(hdc);
//     HBITMAP hBitmap = CreateCompatibleBitmap(
//         hdcCompatible, 
//         GetSystemMetrics(SM_CXSCREEN), 
//         GetSystemMetrics(SM_CYSCREEN)
//     );
//     SelectObject(hdcCompatible, hBitmap);
//     BitBlt(
//         hdcCompatible,
//         0, 0,
//         GetSystemMetrics(SM_CXSCREEN), 
//         GetSystemMetrics(SM_CYSCREEN),
//         hdc,
//         0, 0, SRCCOPY
//     );

//     BITMAP bmp;
//     GetObject(hBitmap, sizeof(BITMAP), &bmp);

//     DeleteDC(hdcCompatible);
//     ReleaseDC(hWnd, hdc);
//     GetDIB
//     return hBitmap;
// }