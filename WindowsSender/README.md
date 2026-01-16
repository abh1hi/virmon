# Windows Indirect Display Driver (IDD) for VirMon

This project implements a Windows Indirect Display Driver (IDD) based on the [Microsoft IddCx sample](https://github.com/microsoft/Windows-driver-samples/tree/master/video/IndirectDisplay).

## Prerequisites
- Visual Studio 2022 with C++ Desktop Development.
- Windows Driver Kit (WDK) for Windows 10/11.

## Building
1. Create a new "Kernel Mode Driver (KMDF)" or "User Mode Driver (UMDF)" project in Visual Studio.
2. Select the "Indirect Display Driver" template if available, or configure the project to link against `IddCx.lib`.
3. Add the `Driver.cpp`, `Driver.h` and `Device.cpp` files.
4. Build the solution (x64).

## Installation
1. Disable Secure Boot or enable Test Signing: `bcdedit /set testsigning on`
2. Install the certificate: `certmgr.exe /add VirMonDriver.cer /s /r localMachine root`
3. Install the driver using `devcon` or Device Manager:
   `devcon install VirMonDriver.inf Root\VirMonDriver`
