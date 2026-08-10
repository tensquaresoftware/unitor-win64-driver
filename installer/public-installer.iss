; Unitor MT4 Bridge — Public Installer (Story 4.1 / AD-12)
; Technology: Inno Setup 6 (community EXE). Compile via scripts/packaging/build-public-installer.ps1
; Branding: Ten Square Software (AD-19). Zadig is not the primary path.
; Operator helpers under installer/*.ps1 are not invoked by this script (SSOT = this .iss).

#ifndef BridgeSource
  #define BridgeSource "..\builds\release\Release"
#endif

#ifndef MyAppVersion
  #define MyAppVersion "0.1.0"
#endif

#define MyAppName "Unitor MT4 Bridge"
#define MyAppPublisher "Ten Square Software"
#define MyAppExeName "Bridge.exe"
#define MyAppId "{{A7C3E91F-4B2D-4E8A-9F1C-6D5E8A3B2C10}}"
#define InstallDirName "Ten Square Software\Unitor MT4 Bridge"
#define MyAppPublisherURL "https://github.com/tensquaresoftware/unitor-win64-driver"
#define MyAppSupportURL "https://github.com/tensquaresoftware/unitor-win64-driver/blob/main/docs/user/README.md"

[Setup]
AppId={#MyAppId}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppPublisherURL}
AppSupportURL={#MyAppSupportURL}
DefaultDirName={autopf}\{#InstallDirName}
DefaultGroupName={#MyAppPublisher}\{#MyAppName}
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
Compression=lzma2
SolidCompression=yes
OutputDir=..\builds\installer
OutputBaseFilename=UnitorMt4Bridge-Setup
SetupLogging=yes
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}
; Do not wipe %LOCALAPPDATA%\unitor-win64-driver\ (unit-identity registry — Story 3.4)
CloseApplications=yes
CloseApplicationsFilter=Bridge.exe
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
WelcomeLabel2=This will install [name/ver] on your computer.%n%nIt associates your Emagic MT4 with WinUSB, installs the Bridge, and registers Auto-Start for your user session.%n%nYou need the VirtualMIDI driver already installed (for example via loopMIDI or rtpMIDI).
FinishedLabel=Setup has finished installing [name] on your computer.%n%nAfter the next sign-in (or when you plug in the MT4), virtual MIDI ports should appear without launching the Bridge by hand.%n%nGetting started: https://github.com/tensquaresoftware/unitor-win64-driver/blob/main/docs/user/README.md
ApplicationsFound=Setup detected that Unitor MT4 Bridge is still running.%n%nContinuing will close it and interrupt any active MIDI session.
ApplicationsFound2=Setup detected that Unitor MT4 Bridge is still running.%n%nContinuing will close it and interrupt any active MIDI session.

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional icons:"; Flags: unchecked

[Files]
; Bridge host (asInvoker manifest is embedded in the binary at build time)
Source: "{#BridgeSource}\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
; WinUSB association package (GUID must match src/Usb/WinUsbTransport.h)
Source: "mt4-winusb.inf"; DestDir: "{app}\drivers"; Flags: ignoreversion
; Operator uninstall helper (also used to record unregister exit status)
Source: "unregister-autostart-user.ps1"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Parameters: "--auto-session"
Name: "{group}\Unregister Auto-Start"; Filename: "{app}\{#MyAppExeName}"; Parameters: "--unregister-auto-start"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Parameters: "--auto-session"; Tasks: desktopicon

[UninstallRun]
; Runs before files are removed. Clears Task Scheduler and HKCU Run (Story 3.1 dual-backend unregister).
; runascurrentuser = interactive user profile, not an elevated admin HKCU hive.
; Helper writes exit code under LocalAppData for post-uninstall verification.
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\unregister-autostart-user.ps1"" -BridgePath ""{app}\{#MyAppExeName}"""; WorkingDir: "{app}"; RunOnceId: "UnregisterAutoStart"; Flags: runascurrentuser waituntilterminated

[Code]
const
  VirtualMidiDll = 'teVirtualMIDI.dll';
  ERROR_SUCCESS_REBOOT_REQUIRED = 3010;
  VirtualMidiFixPath =
    'VirtualMIDI driver/DLL is missing.'#13#10#13#10 +
    'Install loopMIDI or rtpMIDI so the VirtualMIDI driver is present, then run this installer again.'#13#10#13#10 +
    'An empty MIDI port list is not a successful install.'#13#10#13#10 +
    'Licensed VirtualMIDI MSI embedding is a future release gate (OQ-1) and is not shipped in this setup.';
  BridgeRunningWarning =
    'Unitor MT4 Bridge is currently running.'#13#10#13#10 +
    'Continuing will close it and interrupt any active MIDI session.'#13#10#13#10 +
    'Continue with install/upgrade?';

var
  GWinUsbOk: Boolean;
  GAutoStartOk: Boolean;
  GRebootRecommended: Boolean;
  GGatesFailedMessage: string;

function VirtualMidiPresent: Boolean;
begin
  Result := FileExists(ExpandConstant('{sys}\' + VirtualMidiDll));
end;

function AllInstallGatesPassed: Boolean;
begin
  { Smoke / AD-12 success = VirtualMIDI present AND WinUSB AND Auto-Start. }
  Result := VirtualMidiPresent and GWinUsbOk and GAutoStartOk;
end;

function IsBridgeProcessRunning: Boolean;
var
  ResultCode: Integer;
begin
  Result :=
    Exec(
      ExpandConstant('{cmd}'),
      '/C tasklist /FI "IMAGENAME eq Bridge.exe" | find /I "Bridge.exe" > NUL',
      '',
      SW_HIDE,
      ewWaitUntilTerminated,
      ResultCode) and (ResultCode = 0);
end;

function InitializeSetup(): Boolean;
begin
  Result := True;
  GWinUsbOk := False;
  GAutoStartOk := False;
  GRebootRecommended := False;
  GGatesFailedMessage := '';

  if not VirtualMidiPresent then
  begin
    MsgBox(VirtualMidiFixPath, mbError, MB_OK);
    Result := False;
  end;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if CurPageID = wpReady then
  begin
    if IsBridgeProcessRunning then
    begin
      if MsgBox(BridgeRunningWarning, mbConfirmation, MB_YESNO) = IDNO then
        Result := False;
    end;
  end;
end;

function NativePnPUtilPath: string;
begin
  { Prefer native 64-bit pnputil when a 32-bit setup host would otherwise hit SysWOW64. }
  if (not Is64BitInstallMode) and
     FileExists(ExpandConstant('{win}\SysNative\pnputil.exe')) then
    Result := ExpandConstant('{win}\SysNative\pnputil.exe')
  else
    Result := ExpandConstant('{sys}\pnputil.exe');
end;

function BindMt4WinUsb: Boolean;
var
  ResultCode: Integer;
  InfPath: string;
  Cmd: string;
begin
  InfPath := ExpandConstant('{app}\drivers\mt4-winusb.inf');
  if not FileExists(InfPath) then
  begin
    GGatesFailedMessage :=
      'WinUSB association failed: INF not found at ' + InfPath;
    Result := False;
    Exit;
  end;

  Cmd := '/add-driver "' + InfPath + '" /install';
  Log('Running: ' + NativePnPUtilPath + ' ' + Cmd);
  WizardForm.StatusLabel.Caption := 'Associating Emagic MT4 with WinUSB…';
  WizardForm.StatusLabel.Update;

  if not Exec(NativePnPUtilPath, Cmd, '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
  begin
    GGatesFailedMessage :=
      'WinUSB association failed: could not start pnputil.';
    Result := False;
    Exit;
  end;

  { 0 = success; 3010 = reboot required after driver package install. }
  if (ResultCode <> 0) and (ResultCode <> ERROR_SUCCESS_REBOOT_REQUIRED) then
  begin
    GGatesFailedMessage :=
      'WinUSB association failed (pnputil exit ' + IntToStr(ResultCode) + ').' + #13#10 +
      'On clean machines an unsigned INF may be rejected. Lab mitigation: installer/sign-lab-package.ps1. ' +
      'Public Authenticode/catalog policy is Story 4.4. ' +
      'Do not use Zadig as the primary community path.';
    Result := False;
    Exit;
  end;

  if ResultCode = ERROR_SUCCESS_REBOOT_REQUIRED then
    GRebootRecommended := True;

  Result := True;
end;

function RegisterAutoStartAsUser: Boolean;
var
  ResultCode: Integer;
begin
  { Must write HKCU / Task Scheduler for the interactive user — not the elevated admin profile. }
  WizardForm.StatusLabel.Caption := 'Registering Auto-Start for your user session…';
  WizardForm.StatusLabel.Update;

  if not ExecAsOriginalUser(
      ExpandConstant('{app}\{#MyAppExeName}'),
      '--register-auto-start',
      ExpandConstant('{app}'),
      SW_HIDE,
      ewWaitUntilTerminated,
      ResultCode) then
  begin
    GGatesFailedMessage :=
      'Auto-Start registration failed: could not start Bridge as the interactive user.';
    Result := False;
    Exit;
  end;

  if ResultCode <> 0 then
  begin
    GGatesFailedMessage :=
      'Auto-Start registration failed (Bridge exit ' + IntToStr(ResultCode) + ').' + #13#10 +
      'Expected: Task Scheduler task or HKCU Run pointing at the installed Bridge with --auto-session.';
    Result := False;
    Exit;
  end;

  Result := True;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    GWinUsbOk := BindMt4WinUsb;
    if not GWinUsbOk then
    begin
      MsgBox(GGatesFailedMessage, mbError, MB_OK);
      { Abort rolls back copied files / ARP so a failed gate is not left as installed. }
      Abort;
    end;

    GAutoStartOk := RegisterAutoStartAsUser;
    if not GAutoStartOk then
    begin
      MsgBox(GGatesFailedMessage, mbError, MB_OK);
      Abort;
    end;

    if not VirtualMidiPresent then
    begin
      GGatesFailedMessage := VirtualMidiFixPath;
      MsgBox(GGatesFailedMessage, mbError, MB_OK);
      Abort;
    end;
  end;
end;

procedure CurPageChanged(CurPageID: Integer);
var
  SuccessBody: string;
begin
  if CurPageID = wpFinished then
  begin
    if AllInstallGatesPassed then
    begin
      WizardForm.FinishedHeadingLabel.Caption := 'Installation successful';
      SuccessBody :=
        'Unitor MT4 Bridge is installed under Program Files.' + #13#10#13#10 +
        'VirtualMIDI is present; WinUSB association and Auto-Start registration both reported success.' + #13#10 +
        'After sign-in (or when you plug in the MT4), virtual MIDI ports should appear without a manual Bridge launch.' + #13#10#13#10 +
        'Daily use does not require Administrator.' + #13#10#13#10 +
        'Getting started: https://github.com/tensquaresoftware/unitor-win64-driver/blob/main/docs/user/README.md';
      if GRebootRecommended then
        SuccessBody := SuccessBody + #13#10#13#10 +
          'Windows reported that a reboot is recommended to finish driver installation. Reboot before relying on WinUSB.';
      WizardForm.FinishedLabel.Caption := SuccessBody;
    end
    else
    begin
      WizardForm.FinishedHeadingLabel.Caption := 'Installation incomplete';
      WizardForm.FinishedLabel.Caption :=
        'Setup could not complete all install gates.' + #13#10#13#10 +
        GGatesFailedMessage + #13#10#13#10 +
        'This is not a successful community install. Fix the issue above, then retry.' + #13#10 +
        'An empty MIDI port list is not success.';
    end;
  end;
end;

function UnregisterExitCodePath: string;
begin
  { Prefer commonappdata so elevated uninstall Code and runascurrentuser helper share one marker. }
  Result := ExpandConstant('{commonappdata}\Ten Square Software\Unitor MT4 Bridge\last-autostart-unregister.exit');
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  ExitPath: string;
  Lines: TArrayOfString;
  CodeText: string;
  ExitCode: Integer;
begin
  if CurUninstallStep = usPostUninstall then
  begin
    ExitPath := UnregisterExitCodePath;
    if FileExists(ExitPath) then
    begin
      if LoadStringsFromFile(ExitPath, Lines) and (GetArrayLength(Lines) > 0) then
      begin
        CodeText := Trim(Lines[0]);
        ExitCode := StrToIntDef(CodeText, -1);
        if ExitCode <> 0 then
          MsgBox(
            'Auto-Start unregister reported failure (exit ' + IntToStr(ExitCode) + ').' + #13#10 +
            'Bridge files were removed, but Task Scheduler / HKCU Run may still launch a missing binary.' + #13#10 +
            'Remove any leftover Unitor MT4 Bridge Auto-Start entry manually if it remains.',
            mbError,
            MB_OK);
      end;
    end
    else
    begin
      MsgBox(
        'Could not verify Auto-Start unregister (exit marker missing).' + #13#10 +
        'If Bridge still starts at logon, remove the Unitor MT4 Bridge Auto-Start entry manually.',
        mbInformation,
        MB_OK);
    end;
  end;
end;
