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
#define DaemonExeName "qtet-daemon.exe"
; 旧版 WinSW 安装器名称，仅用于旧版本升级时停止/卸载旧服务
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
Filename: "{app}\{#DaemonExeName}"; Parameters: "--install"; StatusMsg: "Installing QtEasyTier daemon service..."; Flags: runhidden waituntilterminated; Check: DaemonExeExists
Filename: "{app}\{#DaemonExeName}"; Parameters: "--start"; StatusMsg: "Starting QtEasyTier daemon service..."; Flags: runhidden waituntilterminated; Check: DaemonExeExists
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{app}\{#DaemonExeName}"; Parameters: "--stop"; Flags: runhidden waituntilterminated; RunOnceId: "StopQtEasyTierDaemon"; Check: DaemonExeExists
Filename: "{app}\{#DaemonExeName}"; Parameters: "--uninstall"; Flags: runhidden waituntilterminated; RunOnceId: "UninstallQtEasyTierDaemon"; Check: DaemonExeExists

[Code]
function DaemonExeExists: Boolean;
begin
  Result := FileExists(ExpandConstant('{app}\{#DaemonExeName}'));
end;

// 通用执行器：目标可执行文件存在时才执行指定命令。
// 用于 ssInstall 阶段在文件复制前清理旧服务，以及旧版 WinSW 迁移。
procedure RunCommandIfExists(const ExeName, Command: String);
var
  ResultCode: Integer;
  ExePath: String;
begin
  ExePath := ExpandConstant('{app}\' + ExeName);
  if FileExists(ExePath) then
  begin
    Exec(ExePath, Command, ExpandConstant('{app}'), SW_HIDE, ewWaitUntilTerminated, ResultCode);
  end;
end;

// CurStepChanged(ssInstall) 在文件复制之前触发（Inno Setup 安装流程中先执行本事件，
// 再执行 InstallDelete/文件复制），因此这里先清理旧服务，释放旧 qtet-daemon.exe
// 的文件占用，随后才能覆盖复制新二进制：
//   1. 新版 qtet-daemon 自我注册的服务（qtet-daemon.sock）：升级时先 --stop 再 --uninstall；
//   2. 旧版 WinSW 服务：仅旧版本升级场景生效（旧 DaemonInstaller.exe 残留时才存在）。
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssInstall then
  begin
    RunCommandIfExists('{#DaemonExeName}', '--stop');
    RunCommandIfExists('{#DaemonExeName}', '--uninstall');
    RunCommandIfExists('{#DaemonInstallerExe}', 'stop');
    RunCommandIfExists('{#DaemonInstallerExe}', 'uninstall');
  end;
end;
