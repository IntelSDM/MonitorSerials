#include <windows.h>
#include <iostream>
#include <setupapi.h>
#include <devguid.h>
#include <vector>
#pragma comment(lib, "setupapi.lib")

#define EDID_SIZE 128

struct MonitorInfo
{
	MonitorInfo(std::string serialNumber) : SerialNumber(serialNumber) {}
	std::string SerialNumber;
};
std::vector<MonitorInfo> MonitorInfos;
void AddMonitorSerialNumber(const BYTE* edidData) 
{
    // Descriptor blocks start at 54
    for (int offset = 54; offset < 126; offset += 18) 
    {
        if (edidData[offset] == 0x00 && edidData[offset + 1] == 0x00 && edidData[offset + 3] == 0xFF)
        {

            std::string serialNumber(reinterpret_cast<const char*>(&edidData[offset + 5]), 13);
            // 0x0a = \n
            serialNumber.erase(std::find_if(serialNumber.rbegin(), serialNumber.rend(),
                [](unsigned char ch) { return ch != ' ' && ch != 0x0A; }).base(), serialNumber.end());
			MonitorInfos.push_back(MonitorInfo(serialNumber));
            return;
        }
    }
    MonitorInfos.push_back(MonitorInfo("0"));
}

void GetMonitorEdid(HDEVINFO hDevInfo, SP_DEVINFO_DATA& devInfoData)
{
    HKEY hKey = SetupDiOpenDevRegKey(hDevInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
    if (hKey != INVALID_HANDLE_VALUE) {
        BYTE edidData[EDID_SIZE] = { 0 };
        DWORD edidSize = sizeof(edidData);
        DWORD type;
        if (RegQueryValueEx(hKey, TEXT("EDID"), NULL, &type, edidData, &edidSize) == ERROR_SUCCESS) 
        {
            AddMonitorSerialNumber(edidData);
        }
        else {
            std::cerr << "Failed to read EDID data from registry. The monitor is likelye an invalid non-generic PNP." << std::endl;
        }
        RegCloseKey(hKey);
    }
    else {
        std::cerr << "Failed to open registry key for device." << std::endl;
    }
}

void EnumerateDisplays() {
    DISPLAY_DEVICE displayDevice;
    displayDevice.cb = sizeof(DISPLAY_DEVICE);
    DWORD deviceIndex = 0;

    while (EnumDisplayDevices(NULL, deviceIndex, &displayDevice, 0)) {
        std::wcout << L"Device Name: " << displayDevice.DeviceName << std::endl;
        std::wcout << L"Device String: " << displayDevice.DeviceString << std::endl;
        std::wcout << L"State Flags: " << displayDevice.StateFlags << std::endl;
		std::wcout << L"Device ID: " << displayDevice.DeviceID << std::endl;
		std::wcout << L"Device Key: " << displayDevice.DeviceKey << std::endl;

        if (displayDevice.StateFlags & DISPLAY_DEVICE_ACTIVE) {
            DISPLAY_DEVICE monitor;
            monitor.cb = sizeof(DISPLAY_DEVICE);
            EnumDisplayDevices(displayDevice.DeviceName, 0, &monitor, 0);
            std::wcout << L"Monitor Name: " << monitor.DeviceString << std::endl;
        }
        deviceIndex++;
    }
}

void main()
{
    EnumerateDisplays();
    HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_MONITOR, NULL, NULL, DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Failed to get device information set for monitors." << std::endl;
        return;
    }

    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); i++) 
    {
        GetMonitorEdid(hDevInfo, devInfoData);
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

	std::cout << "Monitor Serial Numbers:" << std::endl;
	for (const auto& monitorInfo : MonitorInfos) 
    {
		std::cout << monitorInfo.SerialNumber << std::endl;
	}
    std::cin.get();
}