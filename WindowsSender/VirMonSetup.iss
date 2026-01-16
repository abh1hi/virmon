[Setup]
AppName=VirMon Windows Sender
AppVersion=1.0
DefaultDirName={pf}\VirMon
DefaultGroupName=VirMon
OutputBaseFilename=VirMonSetup
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin

[Files]
; Service Executable
Source: "WindowsSender\x64\Release\CaptureService.exe"; DestDir: "{app}"; Flags: ignoreversion

; Driver Files
Source: "WindowsSender\x64\Release\VirMonDriver.inf"; DestDir: "{app}\Driver"; Flags: ignoreversion
Source: "WindowsSender\x64\Release\VirMonDriver.sys"; DestDir: "{app}\Driver"; Flags: ignoreversion
Source: "WindowsSender\x64\Release\VirMonDriver.cat"; DestDir: "{app}\Driver"; Flags: ignoreversion

[Run]
; Install Driver
Filename: "pnputil.exe"; Parameters: "/add-driver ""{app}\Driver\VirMonDriver.inf"" /install"; StatusMsg: "Installing Virtual Display Driver..."; Flags: runhidden waituntilterminated

; Add Firewall Rule
Filename: "netsh.exe"; Parameters: "advfirewall firewall add rule name=""VirMon Streaming"" dir=in action=allow program=""{app}\CaptureService.exe"" enable=yes"; StatusMsg: "Configuring Firewall..."; Flags: runhidden

[UninstallRun]
; Remove Driver
Filename: "pnputil.exe"; Parameters: "/delete-driver ""VirMonDriver.inf"" /uninstall /force"; StatusMsg: "Removing Virtual Display Driver..."; Flags: runhidden

; Remove Firewall Rule
Filename: "netsh.exe"; Parameters: "advfirewall firewall delete rule name=""VirMon Streaming"""; StatusMsg: "Removing Firewall Rules..."; Flags: runhidden
