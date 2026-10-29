#ifndef MyAppVersion
  #define MyAppVersion "0.8.1"
#endif
#ifndef SourceDir
  #define SourceDir "..\bundle"
#endif

#define MyAppName "QuestBridge XR"
#define MyAppPublisher "QuestBridge XR"
#define MyAppExeName "QuestBridgeStudio.exe"

[Setup]
AppId={{B10D9D16-24D4-4909-8E2A-25F1C17C7F01}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={localappdata}\Programs\QuestBridgeXR
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
OutputDir=..\installer-output
OutputBaseFilename=QuestBridgeXR-{#MyAppVersion}-Setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#MyAppName}
Uninstallable=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64
CloseApplications=force
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Files]
; Preserve user configuration and named profiles when upgrading/reinstalling. New files are seeded only if absent.
Source: "{#SourceDir}\*"; DestDir: "{app}"; Excludes: "config\questbridge.json;config\profiles\*.json"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\config\questbridge.json"; DestDir: "{app}\config"; Flags: onlyifdoesntexist uninsneveruninstall
Source: "{#SourceDir}\config\profiles\*"; DestDir: "{app}\config\profiles"; Flags: onlyifdoesntexist recursesubdirs createallsubdirs uninsneveruninstall

[Icons]
Name: "{group}\QuestBridge Studio"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"
Name: "{autodesktop}\QuestBridge Studio"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch QuestBridge Studio"; Flags: postinstall nowait skipifsilent
