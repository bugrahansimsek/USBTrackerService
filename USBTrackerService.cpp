#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <dbt.h>
#include <setupapi.h>
#include <tchar.h>
#include <strsafe.h>
#include <InitGuid.h>
#include <usbiodef.h>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

#pragma comment(lib, "Setupapi.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

const wchar_t* SERVICE_NAME = L"USBTrackerService";
const wchar_t* LOG_FILE_PATH = L"C:\\USBTrackerLog.txt";

SERVICE_STATUS g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = nullptr;
HANDLE g_ServiceStopEvent = INVALID_HANDLE_VALUE;

// Fonksiyon prototipleri
VOID WINAPI ServiceMain(DWORD argc, LPWSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode);
LRESULT CALLBACK HiddenWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void WriteLog(const std::wstring& message);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);
std::wstring ParseUsbDetails(const std::wstring& devicePath);
std::wstring GetComputerName();
std::wstring GetIPAddress();

void WriteLog(const std::wstring& message) {
    HANDLE hFile = CreateFileW(
        LOG_FILE_PATH,
        FILE_APPEND_DATA,
        FILE_SHARE_READ,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten;
        std::wstring logEntry = message + L"\r\n";
        WriteFile(hFile, logEntry.c_str(), static_cast<DWORD>(logEntry.size() * sizeof(wchar_t)), &bytesWritten, nullptr);
        CloseHandle(hFile);
    }
}

std::wstring GetComputerName() {
    WCHAR buffer[MAX_COMPUTERNAME_LENGTH + 1] = { 0 };
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    if (!GetComputerNameW(buffer, &size)) {
        return L"UNKNOWN-PC";
    }
    return std::wstring(buffer);
}

std::wstring GetIPAddress() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return L"0.0.0.0";
    }

    PIP_ADAPTER_ADDRESSES addresses = nullptr;
    ULONG bufferSize = 15 * 1024;
    std::wstring result = L"0.0.0.0";

    for (int i = 0; i < 3; i++) {
        addresses = (IP_ADAPTER_ADDRESSES*)malloc(bufferSize);
        if (GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST, nullptr, addresses, &bufferSize) == ERROR_SUCCESS) {
            break;
        }
        free(addresses);
        addresses = nullptr;
    }

    if (addresses) {
        PIP_ADAPTER_ADDRESSES addr = addresses;
        while (addr) {
            if (addr->OperStatus == IfOperStatusUp && addr->FirstUnicastAddress) {
                SOCKET_ADDRESS sa = addr->FirstUnicastAddress->Address;
                WCHAR ipStr[46];
                DWORD ipStrSize = sizeof(ipStr) / sizeof(ipStr[0]);
                if (WSAAddressToStringW(sa.lpSockaddr, sa.iSockaddrLength, nullptr, ipStr, &ipStrSize) == 0) {
                    result = ipStr;
                    size_t colonPos = result.find(L':');
                    if (colonPos != std::wstring::npos) {
                        result = result.substr(0, colonPos);
                    }
                    break;
                }
            }
            addr = addr->Next;
        }
        free(addresses);
    }

    WSACleanup();
    return result;
}

std::wstring ParseUsbDetails(const std::wstring& devicePath) {
    std::wstringstream ss;
    std::vector<std::wstring> parts;
    std::wstring temp;
    std::wistringstream iss(devicePath);

    while (std::getline(iss, temp, L'#')) {
        if (!temp.empty()) parts.push_back(temp);
    }

    std::wstring vid = L"", pid = L"", serial = L"";
    if (parts.size() > 1) {
        size_t vid_pos = parts[1].find(L"VID_");
        size_t pid_pos = parts[1].find(L"PID_");

        if (vid_pos != std::wstring::npos && vid_pos + 4 < parts[1].length())
            vid = parts[1].substr(vid_pos + 4, 4);
        if (pid_pos != std::wstring::npos && pid_pos + 4 < parts[1].length())
            pid = parts[1].substr(pid_pos + 4, 4);
    }

    if (parts.size() > 2) serial = parts[2];

    HDEVINFO devInfo = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_USB_DEVICE, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    std::wstring manufacturer = L"Unknown Manufacturer";
    std::wstring deviceDesc = L"Unknown Device";

    if (devInfo != INVALID_HANDLE_VALUE) {
        SP_DEVINFO_DATA devInfoData = { sizeof(SP_DEVINFO_DATA) };

        for (DWORD i = 0; SetupDiEnumDeviceInfo(devInfo, i, &devInfoData); i++) {
            WCHAR buffer[256] = { 0 };

            if (SetupDiGetDeviceRegistryPropertyW(devInfo, &devInfoData, SPDRP_MFG, nullptr,
                reinterpret_cast<PBYTE>(buffer), sizeof(buffer), nullptr)) {
                manufacturer = buffer;
            }

            if (SetupDiGetDeviceRegistryPropertyW(devInfo, &devInfoData, SPDRP_DEVICEDESC, nullptr,
                reinterpret_cast<PBYTE>(buffer), sizeof(buffer), nullptr)) {
                deviceDesc = buffer;
            }

            break;
        }
        SetupDiDestroyDeviceInfoList(devInfo);
    }

    ss << L"PC: " << GetComputerName()
        << L" | IP: " << GetIPAddress()
        << L" | Device: " << deviceDesc
        << L" | Manufacturer: " << manufacturer
        << L" | VID: " << vid
        << L" | PID: " << pid
        << L" | Serial: " << serial;

    return ss.str();
}

LRESULT CALLBACK HiddenWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DEVICECHANGE) {
        PDEV_BROADCAST_HDR pHdr = reinterpret_cast<PDEV_BROADCAST_HDR>(lParam);
        if (pHdr && pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
            PDEV_BROADCAST_DEVICEINTERFACE pDevInf = reinterpret_cast<PDEV_BROADCAST_DEVICEINTERFACE>(pHdr);

            std::wstring eventType;
            switch (wParam) {
            case DBT_DEVICEARRIVAL:
                eventType = L"[+] USB Connected: ";
                break;
            case DBT_DEVICEREMOVECOMPLETE:
                eventType = L"[-] USB Disconnected: ";
                break;
            default:
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }

            WriteLog(eventType + ParseUsbDetails(pDevInf->dbcc_name));
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam) {
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = HiddenWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"USBTrackerHiddenWindow";

    if (!RegisterClassW(&wc)) {
        WriteLog(L"Window class registration failed");
        return 1;
    }

    HWND hwnd = CreateWindowW(
        wc.lpszClassName,
        L"USB Tracker Hidden Window",
        0, 0, 0, 0, 0,
        HWND_MESSAGE,
        nullptr,
        wc.hInstance,
        nullptr);

    if (!hwnd) {
        WriteLog(L"Window creation failed");
        return 1;
    }

    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter = { 0 };
    NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;

    HDEVNOTIFY hDevNotify = RegisterDeviceNotification(
        hwnd,
        &NotificationFilter,
        DEVICE_NOTIFY_WINDOW_HANDLE);

    if (!hDevNotify) {
        WriteLog(L"Device notification registration failed");
        return 1;
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    UnregisterDeviceNotification(hDevNotify);
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode) {
    switch (CtrlCode) {
    case SERVICE_CONTROL_STOP:
        WriteLog(L"Service stopping...");
        SetEvent(g_ServiceStopEvent);
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        break;
    default:
        break;
    }
}

VOID WINAPI ServiceMain(DWORD argc, LPWSTR* argv) {
    g_StatusHandle = RegisterServiceCtrlHandlerW(SERVICE_NAME, ServiceCtrlHandler);
    if (!g_StatusHandle) return;

    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    g_ServiceStopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_ServiceStopEvent) {
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        return;
    }

    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    WriteLog(L"Service started");

    HANDLE hThread = CreateThread(nullptr, 0, ServiceWorkerThread, nullptr, 0, nullptr);
    WaitForSingleObject(g_ServiceStopEvent, INFINITE);

    CloseHandle(hThread);
    CloseHandle(g_ServiceStopEvent);

    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
    WriteLog(L"Service stopped");
}

int wmain(int argc, wchar_t* argv[]) {
    SERVICE_TABLE_ENTRYW DispatchTable[] = {
        {const_cast<LPWSTR>(SERVICE_NAME), ServiceMain},
        {nullptr, nullptr}
    };

    if (!StartServiceCtrlDispatcherW(DispatchTable)) {
        WriteLog(L"Service control dispatcher failed");
    }
    return 0;
}