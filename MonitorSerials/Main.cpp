#include <windows.h>
#include <iostream>
#include <setupapi.h>
#include <devguid.h>

#pragma comment(lib, "setupapi.lib")

#define EDID_SIZE 128

void PrintMonitorEdid(const BYTE* edidData) 
{
    // Descriptor blocks start at 54
    for (int offset = 54; offset < 126; offset += 18) {
        if (edidData[offset] == 0x00 && edidData[offset + 1] == 0x00 && edidData[offset + 3] == 0xFF) {
            std::string serialNumber(reinterpret_cast<const char*>(&edidData[offset + 5]), 13);
            // 0x0a = \n
            serialNumber.erase(std::find_if(serialNumber.rbegin(), serialNumber.rend(),
                [](unsigned char ch) { return ch != ' ' && ch != 0x0A; }).base(), serialNumber.end());
            std::cout << "Monitor Serial Number: " << serialNumber << std::endl;
            return;
        }
    }
    std::cout << "Serial number not found" << std::endl;
}

void GetMonitorEdid(HDEVINFO hDevInfo, SP_DEVINFO_DATA& devInfoData)
{
    HKEY hKey = SetupDiOpenDevRegKey(hDevInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
    if (hKey != INVALID_HANDLE_VALUE) {
        BYTE edidData[EDID_SIZE] = { 0 };
        DWORD edidSize = sizeof(edidData);
        DWORD type;
        if (RegQueryValueEx(hKey, TEXT("EDID"), NULL, &type, edidData, &edidSize) == ERROR_SUCCESS) {
            std::cout << "Successfully read EDID data from registry." << std::endl;
            PrintMonitorEdid(edidData);
        }
        else 
        {
            std::cerr << "Failed to read EDID data from registry." << std::endl;
        }
        RegCloseKey(hKey);
    }
    else
    {
        std::cerr << "Failed to open registry key for device." << std::endl;
    }
}

void main()
{
    HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_MONITOR, NULL, NULL, DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to get device information set for monitors." << std::endl;
        return;
    }

    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); i++) {
        GetMonitorEdid(hDevInfo, devInfoData);
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    return;
}