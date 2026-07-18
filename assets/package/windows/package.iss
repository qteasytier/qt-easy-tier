; QtEasyTier Windows installer script.
; Build-time values are provided by ISCC /D arguments from CI or local scripts.

#ifndef MyAppVersion
  #error MyAppVersion is required
#endif

#ifndef BuildOutputDir
  #error BuildOutputDir is required
#endif

#ifndef PackageOutputDir
  #error PackageOutputDir is required
#endif

#ifndef SourceRootDir
  #error SourceRootDir is required
#endif

#define MyAppName "QtEasyTier"
#define MyAppPublisher "Myqfeng"
#define MyAppURL "https://qtet.cn"
#define MyAppExeName "appQtEasyTier.exe"
#define DaemonInstallerExe "DaemonInstaller.exe"

[Setup]
AppId={{A55529DE-9970-4A63-BE82-72DF34283AAC}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
AllowNoIcons=yes
LicenseFile={#SourceRootDir}\LICENSE
OutputDir={#PackageOutputDir}
OutputBaseFilename=qteasytier_v{#MyAppVersion}_windows_amd64
SetupIconFile={#SourceRootDir}\assets\favicon\qtet.ico
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "chinesesimplified"; MessagesFile: "{#SourceRootDir}\assets\package\windows\ChineseSimplified.isl"
Name: "chinesetraditional"; MessagesFile: "{#SourceRootDir}\assets\package\windows\ChineseTraditional.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#BuildOutputDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#DaemonInstallerExe}"; Parameters: "install"; StatusMsg: "Installing QtEasyTier daemon service..."; Flags: runhidden waituntilterminated; Check: DaemonInstallerExists
Filename: "{app}\{#DaemonInstallerExe}"; Parameters: "start"; StatusMsg: "Starting QtEasyTier daemon service..."; Flags: runhidden waituntilterminated; Check: DaemonInstallerExists
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{app}\{#DaemonInstallerExe}"; Parameters: "stop"; Flags: runhidden waituntilterminated; RunOnceId: "StopQtEasyTierDaemon"; Check: DaemonInstallerExists
Filename: "{app}\{#DaemonInstallerExe}"; Parameters: "uninstall"; Flags: runhidden waituntilterminated; RunOnceId: "UninstallQtEasyTierDaemon"; Check: DaemonInstallerExists

[Code]
function DaemonInstallerExists: Boolean;
begin
  Result := FileExists(ExpandConstant('{app}\{#DaemonInstallerExe}'));
end;

procedure RunDaemonCommandIfExists(const Command: String);
var
  ResultCode: Integer;
  InstallerPath: String;
begin
  InstallerPath := ExpandConstant('{app}\{#DaemonInstallerExe}');
  if FileExists(InstallerPath) then
  begin
    Exec(InstallerPath, Command, ExpandConstant('{app}'), SW_HIDE, ewWaitUntilTerminated, ResultCode);
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssInstall then
  begin
    RunDaemonCommandIfExists('stop');
    RunDaemonCommandIfExists('uninstall');
  end;
end;
