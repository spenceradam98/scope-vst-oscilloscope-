; Windows installer for Scope (Inno Setup 6).
; Build: iscc /DAppVersion=0.1.0 /DArtefacts=<path to Scope_artefacts\Release> /DOutDir=<dir> Scope.iss

#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif
#ifndef Artefacts
  #define Artefacts "..\..\build\Scope_artefacts\Release"
#endif
#ifndef OutDir
  #define OutDir "..\..\dist"
#endif

[Setup]
AppId={{6B7C2E4A-3F1D-4C8E-9A51-5C0D2B7E9F13}
AppName=Scope
AppVersion={#AppVersion}
AppPublisher=YourBrand
DefaultDirName={autopf}\YourBrand\Scope
DefaultGroupName=YourBrand
DisableProgramGroupPage=yes
OutputDir={#OutDir}
OutputBaseFilename=Scope-{#AppVersion}-windows-setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
UninstallDisplayName=Scope {#AppVersion}
InfoBeforeFile=readme-before.txt

[Types]
Name: "full"; Description: "Full install"
Name: "custom"; Description: "Custom"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plugin (Ableton, FL Studio, Cubase, Reaper, Studio One...)"; Types: full custom; Flags: fixed
Name: "app"; Description: "Standalone app"; Types: full

[Files]
; VST3 goes to the standard shared folder every DAW scans
Source: "{#Artefacts}\VST3\Scope.vst3\*"; DestDir: "{commoncf64}\VST3\Scope.vst3"; Components: vst3; Flags: recursesubdirs createallsubdirs ignoreversion
Source: "{#Artefacts}\Standalone\Scope.exe"; DestDir: "{app}"; Components: app; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\Scope"; Filename: "{app}\Scope.exe"; Components: app
Name: "{autodesktop}\Scope"; Filename: "{app}\Scope.exe"; Components: app; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut for the standalone app"; Components: app; Flags: unchecked

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Scope.vst3"
