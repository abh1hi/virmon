# Building and Installing the VirMon Virtual Display Driver

## Prerequisites

Before you can use VirMon, you need to build the virtual display driver. This requires:

1. **Visual Studio 2019 or later** with C++ Desktop Development workload
2. **Windows Driver Kit (WDK)** - [Download from Microsoft](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk)
3. **Windows SDK** (usually installed with Visual Studio)

## Step-by-Step Instructions

### Option 1: Build Using Visual Studio (Recommended)

1. **Open the Driver Project**
   - Navigate to: `c:\Users\abhin\Desktop\virmon\WindowsSender`
   - Double-click `VirMonDriver.vcxproj` to open in Visual Studio

2. **Set Build Configuration**
   - At the top toolbar, change configuration dropdown to: **Release**
   - Change platform dropdown to: **x64**

3. **Build the Driver**
   - Press `Ctrl+Shift+B` or go to `Build > Build Solution`
   - Wait for the build to complete
   - Check the output window for any errors

4. **Verify the Build**
   - The driver should be created at:
     ```
     c:\Users\abhin\Desktop\virmon\WindowsSender\x64\Release\VirMonDriver\VirMonDriver.sys
     ```

### Option 2: Build Using Command Line

1. **Open Developer Command Prompt for VS**
   - Start Menu > Visual Studio 2019/2022 > Developer Command Prompt

2. **Navigate to Driver Directory**
   ```cmd
   cd c:\Users\abhin\Desktop\virmon\WindowsSender
   ```

3. **Build with MSBuild**
   ```cmd
   msbuild VirMonDriver.vcxproj /p:Configuration=Release /p:Platform=x64
   ```

## Installing the Driver

Once the driver is built:

1. **Right-click** on `install_driver.ps1` and select **Run with PowerShell**
2. The script will:
   - Verify the driver binary exists
   - Check and enable test signing (requires restart)
   - Install the driver
   - Verify installation

3. **If test signing was just enabled**: Restart your computer

4. **After restart**, check Device Manager:
   - Press `Win+X` and select "Device Manager"
   - Look under "Display adapters" for "VirMon Virtual Display"
   - Status should show "This device is working properly"

## Troubleshooting

### Error: "VirMonDriver.sys not found"
- **Solution**: Build the driver using one of the methods above

### Error: "Test signing is not enabled"
- **Solution**: Run `install_driver.ps1` which will enable it automatically
- **Note**: Restart required after enabling test signing

### Error: "Driver failed to load" or "CM_PROB_FAILED_ADD"
- **Cause**: Usually test signing not enabled or system not restarted
- **Solution**: 
  1. Enable test signing: `bcdedit /set testsigning on` (as Administrator)
  2. Restart Windows
  3. Run `install_driver.ps1` again

### Virtual monitor doesn't appear in Display Settings
- **Check**: Device Manager shows driver status as "OK"
- **Try**: Right-click desktop > Display Settings > Detect displays
- **Verify**: Run the sender application (`VirMonSender.exe`) - it initializes the monitor

## Next Steps

After successful driver installation:

1. **Build the Sender Application**
   ```powershell
   cd c:\Users\abhin\Desktop\virmon\WindowsSender
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release
   ```

2. **Build the Android App**
   - Open `c:\Users\abhin\Desktop\virmon\AndroidReceiver` in Android Studio
   - Build and install on your Android device

3. **Test the System**
   - Run `VirMonSender.exe` on Windows
   - Launch the VirMon app on Android
   - They should auto-discover each other on the same WiFi network
