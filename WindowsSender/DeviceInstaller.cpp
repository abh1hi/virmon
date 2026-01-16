#include <windows.h>
#include <setupapi.h>
#include <newdev.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "newdev.lib")

// GUID for Display Class
// {4d36e968-e325-11ce-bfc1-08002be10318}
DEFINE_GUID(GUID_DEVCLASS_DISPLAY, 
0x4d36e968, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18);

bool CreateSoftwareDevice(const std::wstring& hardwareId, const std::wstring& infPath) {
    std::cout << "Creating Software Device Node: " << std::string(hardwareId.begin(), hardwareId.end()) << "..." << std::endl;

    HDEVINFO deviceInfoSet = SetupDiCreateDeviceInfoList(&GUID_DEVCLASS_DISPLAY, NULL);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        std::cerr << "SetupDiCreateDeviceInfoList failed. Error: " << GetLastError() << std::endl;
        return false;
    }

    SP_DEVINFO_DATA deviceInfoData;
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    // Create the device node
    if (!SetupDiCreateDeviceInfoW(deviceInfoSet, L"Display", &GUID_DEVCLASS_DISPLAY, 
                                 NULL, NULL, DICD_GENERATE_ID, &deviceInfoData)) {
        std::cerr << "SetupDiCreateDeviceInfoW failed. Error: " << GetLastError() << std::endl;
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return false;
    }

    // Set the HardwareID
    std::vector<wchar_t> hwIdBuffer;
    hwIdBuffer.insert(hwIdBuffer.end(), hardwareId.begin(), hardwareId.end());
    hwIdBuffer.push_back(0); // Null terminator
    hwIdBuffer.push_back(0); // Multi-sz double null

    if (!SetupDiSetDeviceRegistryPropertyW(deviceInfoSet, &deviceInfoData, SPDRP_HARDWAREID, 
                                          (const BYTE*)hwIdBuffer.data(), (DWORD)hwIdBuffer.size() * sizeof(wchar_t))) {
        std::cerr << "SetupDiSetDeviceRegistryPropertyW failed. Error: " << GetLastError() << std::endl;
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return false;
    }

    // Register the device (This creates the "Unknown Device" in Device Manager)
    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE, deviceInfoSet, &deviceInfoData)) {
        std::cerr << "SetupDiCallClassInstaller(REGISTERDEVICE) failed. Error: " << GetLastError() << std::endl;
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return false;
    }

    std::cout << "Device Node created successfully." << std::endl;

    // Now update the driver for this device using the INF
    // UpdateDriverForPlugAndPlayDevices works on the Hardware ID
    BOOL rebootRequired = FALSE;
    std::cout << "Updating Driver using INF: " << std::string(infPath.begin(), infPath.end()) << "..." << std::endl;
    
    if (!UpdateDriverForPlugAndPlayDevicesW(NULL, hardwareId.c_str(), infPath.c_str(), INSTALLFLAG_FORCE, &rebootRequired)) {
        std::cerr << "UpdateDriverForPlugAndPlayDevicesW failed. Error: " << GetLastError() << std::endl;
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return false;
    }

    std::cout << "Driver Installed Successfully!" << std::endl;
    if (rebootRequired) {
        std::cout << "WARNING: A reboot is required." << std::endl;
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return true;
}

int main(int argc, char* argv[]) {
    std::cout << "VirMon Device Installer" << std::endl;
    std::cout << "-----------------------" << std::endl;

    // Must map to the Hardware ID in the INF [Standard.NT$ARCH$] section
    std::wstring hwId = L"ROOT\\VirMonDriver";
    
    std::wstring infPath;

    if (argc > 1) {
        // Use provided argument (Simple conversion, assumes ASCII path for now)
        // For robustness with spaces, we should use GetCommandLineW and CommandLineToArgvW, but simple argv works if quoted correctly by PS.
        std::string arg1 = argv[1];
        infPath = std::wstring(arg1.begin(), arg1.end());
    } else {
        // Fallback
        wchar_t currentDir[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, currentDir);
        infPath = std::wstring(currentDir) + L"\\VirMonDriver.inf";
    }

    if (CreateSoftwareDevice(hwId, infPath)) {
        return 0;
    } else {
        std::cout << "Installation failed." << std::endl;
        return 1;
    }
}
