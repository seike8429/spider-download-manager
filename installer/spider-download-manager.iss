; Spider Download Manager installer (Inno Setup 6).
;
; Built by GitHub Actions (see .github/workflows/build.yml): Inno Setup is
; installed with Chocolatey, then ISCC compiles this script. The dist/ folder
; (exe + Qt runtime, produced by windeployqt) and browser-extension/ (the
; Chrome/Edge extension) are packaged together. Per-user install, no admin
; rights needed: the app lives in %LOCALAPPDATA%\Programs, shortcuts go to
; the user's Start menu and (optionally) desktop.
;
; Paths are relative to this script's folder (installer/).

#define AppName "Spider Download Manager"
#define AppVersion "1.3.1"
#define AppExe "Spider Download Manager.exe"

[Setup]
AppId={{CA33555F-EDCD-4F7B-99DF-EC042DBB4B76}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=Spider
AppPublisherURL=https://github.com/seike8429/spider-download-manager
DefaultDirName={localappdata}\Programs\{#AppName}
DefaultGroupName={#AppName}
PrivilegesRequired=lowest
OutputDir=installer-out
OutputBaseFilename=SpiderDownloadManager-Setup-{#AppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\{#AppExe}
SetupIconFile=..\src\app.ico

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "startup"; Description: "Start with Windows (minimized in the notification area)"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; The app and its Qt runtime (dist/ is produced by windeployqt in CI).
Source: "..\dist\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; yt-dlp.exe (v1.2): bundled so a fresh install can fetch video qualities
; immediately. The app also auto-downloads it on first use (see
; InstallYtDlp), so an older install heals itself the same way.
Source: "..\tools\yt-dlp.exe"; DestDir: "{app}"; Flags: ignoreversion
; aria2c.exe (v1.3.1): yt-dlp's external downloader for fast YouTube
; downloads (16 parallel connections, IDM-style). The app also
; auto-downloads it on first use (see InstallAria2c), so an older install
; heals itself the same way.
Source: "..\tools\aria2c.exe"; DestDir: "{app}"; Flags: ignoreversion
; The browser extension, so the app can offer a one-click install helper.
Source: "..\browser-extension\*"; DestDir: "{app}\extension"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Registry]
; "Start with Windows" task: the same HKCU Run entry the app itself writes
; from Options > Startup. The app keeps it in sync if the folder moves.
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "{#AppName}"; ValueData: """{app}\{#AppExe}"" --tray"; Flags: uninsdeletevalue; Tasks: startup

[Run]
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,{#StringChange(AppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
