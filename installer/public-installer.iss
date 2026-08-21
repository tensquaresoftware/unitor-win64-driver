; Unitor MT4 Bridge — Public Installer (Stories 4.1 / 6.2)
; Technology: Inno Setup 6 (community EXE). Compile via scripts/packaging/build-public-installer.ps1
; Branding: Ten Square Software (AD-19). Guided WinUSB (Zadig) when Setup-alone bind fails — not Zadig-primary UX.
; Operator helpers under installer/*.ps1 are not invoked by this script (SSOT = this .iss).
;
; Dual community flavors (same AppId; same Program Files tree — Ask First left unanswered = no side-by-side AppIds):
;   win11-wms          — Windows MIDI Services comfort path (default); no teVirtualMIDI.dll gate
;   win10-virtualmidi  — virtualMIDI self-install path; DLL presence gate; never embed MSI/DLL

#ifndef BridgeSource
  #define BridgeSource "..\builds\release\Release"
#endif

#ifndef MyAppVersion
  #define MyAppVersion "0.1.0"
#endif

; Four-part PE VERSIONINFO (Explorer File version). Defaults from MyAppVersion + ".0".
#ifndef MyAppVersionInfo
  #define MyAppVersionInfo MyAppVersion ".0"
#endif

; Flavor token for artifact names (win11-wms | win10-virtualmidi).
#ifndef FlavorToken
  #define FlavorToken "win11-wms"
#endif

; 1 = require teVirtualMIDI.dll (Win10 flavor); 0 = WMS path (Win11 flavor).
#ifndef RequireVirtualMidi
  #define RequireVirtualMidi 0
#endif

; Default MidiBackend CLI value baked into shortcuts + Auto-Start registration.
#ifndef MidiBackendArg
  #define MidiBackendArg "wms"
#endif

; Compile-time flavor consistency: RequireVirtualMidi and MidiBackendArg must agree.
#if RequireVirtualMidi
  #if MidiBackendArg != "virtualmidi"
    #error FlavorConsistency: RequireVirtualMidi=1 requires MidiBackendArg=virtualmidi
  #endif
#else
  #if MidiBackendArg != "wms"
    #error FlavorConsistency: RequireVirtualMidi=0 requires MidiBackendArg=wms
  #endif
#endif

#define MyAppName "Unitor MT4 Bridge"
#define MyAppPublisher "Ten Square Software"
#define MyAppExeName "Bridge.exe"
#define MyAppId "{{A7C3E91F-4B2D-4E8A-9F1C-6D5E8A3B2C10}}"
#define InstallDirName "Ten Square Software\Unitor MT4 Bridge"
#define MyAppPublisherURL "https://github.com/tensquaresoftware/unitor-win64-driver"
#define MyAppSupportURL "https://github.com/tensquaresoftware/unitor-win64-driver/blob/main/docs/user/README.md"
; Luthier-style product-version-flavor naming for GitHub Release assets.
#define SetupBaseName "Unitor-MT4-Bridge-" + MyAppVersion + "-" + FlavorToken + "-setup"
#define BridgeSessionParams "--auto-session --midi-backend=" + MidiBackendArg
#define BridgeRegisterParams "--register-auto-start --midi-backend=" + MidiBackendArg

[Setup]
AppId={#MyAppId}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppPublisherURL}
AppSupportURL={#MyAppSupportURL}
; Explorer "File version" / Properties — AppVersion alone does not fill PE VERSIONINFO.
VersionInfoVersion={#MyAppVersionInfo}
VersionInfoProductVersion={#MyAppVersion}
VersionInfoCompany={#MyAppPublisher}
VersionInfoProductName={#MyAppName}
DefaultDirName={autopf}\{#InstallDirName}
DefaultGroupName={#MyAppPublisher}\{#MyAppName}
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
Compression=lzma2
SolidCompression=yes
; Compiled Setup EXEs land in dist/ (gitignored), like Luthier dist/ build output.
OutputDir=..\dist
OutputBaseFilename={#SetupBaseName}
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
#if RequireVirtualMidi
WelcomeLabel2=This will install [name/ver] on your computer.%n%nIt associates your Emagic MT4 with WinUSB, installs the Bridge, and registers Auto-Start for your user session.%n%nYou need the virtualMIDI driver already installed (for example via loopMIDI or rtpMIDI). This project never embeds teVirtualMIDI.dll or a virtualMIDI MSI.
FinishedLabel=Setup has finished installing [name] on your computer.%n%nAfter the next sign-in (or when you plug in the MT4), virtual MIDI ports should appear without launching the Bridge by hand.%n%nGetting started: https://github.com/tensquaresoftware/unitor-win64-driver/blob/main/docs/user/README.md
#else
WelcomeLabel2=This will install [name/ver] on your computer.%n%nIt associates your Emagic MT4 with WinUSB, installs the Bridge (Windows MIDI Services path), and registers Auto-Start for your user session.%n%nYou do not need virtualMIDI. Windows 11 with Windows MIDI Services is required. An empty MIDI port list is not a successful install.
FinishedLabel=Setup has finished installing [name] on your computer.%n%nAfter the next sign-in (or when you plug in the MT4), MIDI ports should appear via Windows MIDI Services without launching the Bridge by hand.%n%nGetting started: https://github.com/tensquaresoftware/unitor-win64-driver/blob/main/docs/user/README.md
#endif
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
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Parameters: "{#BridgeSessionParams}"
Name: "{group}\Unregister Auto-Start"; Filename: "{app}\{#MyAppExeName}"; Parameters: "--unregister-auto-start"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Parameters: "{#BridgeSessionParams}"; Tasks: desktopicon

[UninstallRun]
; Runs before files are removed. Clears Task Scheduler and HKCU Run (Story 3.1 dual-backend unregister).
; runascurrentuser = interactive user profile, not an elevated admin HKCU hive.
; Helper writes exit code under LocalAppData for post-uninstall verification.
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\unregister-autostart-user.ps1"" -BridgePath ""{app}\{#MyAppExeName}"""; WorkingDir: "{app}"; RunOnceId: "UnregisterAutoStart"; Flags: runascurrentuser waituntilterminated

[Code]
const
  VirtualMidiDll = 'teVirtualMIDI.dll';
  ERROR_SUCCESS_REBOOT_REQUIRED = 3010;
  RequireVirtualMidiGate = {#RequireVirtualMidi};
  VirtualMidiFixPath =
    'virtualMIDI driver/DLL is missing.'#13#10#13#10 +
    'Install virtualMIDI yourself from Tobias Erichsen (for example via loopMIDI or rtpMIDI) so teVirtualMIDI.dll is present in System32, then run this installer again.'#13#10#13#10 +
    'An empty MIDI port list is not a successful install.'#13#10#13#10 +
    'This project never embeds teVirtualMIDI.dll or a virtualMIDI MSI (OQ-1).';
  WmsPathHonesty =
    'This Setup uses the Windows MIDI Services path (Win11 community flavor).'#13#10 +
    'virtualMIDI is not required and is not redistributed.';
  WmsOsRequired =
    'This Setup is the Windows 11 / Windows MIDI Services community flavor.'#13#10#13#10 +
    'Windows 11 is required. On Windows 10, download the win10-virtualmidi Setup instead '#13#10 +
    '(and self-install virtualMIDI). See docs/user/README.md.';
  WmsUnavailable =
    'Windows MIDI Services does not appear to be available on this PC '#13#10 +
    '(midisrv service not found).'#13#10#13#10 +
    'An empty MIDI port list after install is not a successful community install.'#13#10 +
    'Enable or install Windows MIDI Services on Windows 11, then run this Setup again.'#13#10#13#10 +
    'Or use the win10-virtualmidi Setup with a user-installed virtualMIDI driver.';
  EmptyPortsNotSuccess =
    'An empty MIDI port list is never a successful install for this project.';
  BridgeRunningWarning =
    'Unitor MT4 Bridge is currently running.'#13#10#13#10 +
    'Continuing will close it and interrupt any active MIDI session.'#13#10#13#10 +
    'Continue with install/upgrade?';

var
  GWinUsbOk: Boolean;
  GAutoStartOk: Boolean;
  GHadAutoStartBefore: Boolean;
  GRebootRecommended: Boolean;
  GGatesFailedMessage: string;
  GDriverStoreMayRemain: Boolean;
  GWmsPrereqOk: Boolean;

function VirtualMidiPresent: Boolean;
begin
  Result := FileExists(ExpandConstant('{sys}\' + VirtualMidiDll));
end;

function VirtualMidiGateRequired: Boolean;
begin
  Result := RequireVirtualMidiGate <> 0;
end;

function IsWindows11OrNewer: Boolean;
var
  Version: TWindowsVersion;
begin
  GetWindowsVersionEx(Version);
  { Win11 reports Major=10 with Build >= 22000. }
  Result := (Version.Major > 10) or
    ((Version.Major = 10) and (Version.Build >= 22000));
end;

function WmsServicePresent: Boolean;
var
  ResultCode: Integer;
begin
  { Fail closed when midisrv is absent — better than “success” then empty ports at logon. }
  Result :=
    Exec(
      ExpandConstant('{cmd}'),
      '/C sc query midisrv >NUL 2>&1',
      '',
      SW_HIDE,
      ewWaitUntilTerminated,
      ResultCode) and (ResultCode = 0);
end;

function AllInstallGatesPassed: Boolean;
begin
  { Win10 flavor: virtualMIDI present AND WinUSB AND Auto-Start.
    Win11 WMS flavor: Win11+WMS prereq AND WinUSB AND Auto-Start (no VirtualMidiPresent). }
  if VirtualMidiGateRequired then
    Result := VirtualMidiPresent and GWinUsbOk and GAutoStartOk
  else
    Result := GWmsPrereqOk and GWinUsbOk and GAutoStartOk;
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
  GHadAutoStartBefore := False;
  GRebootRecommended := False;
  GGatesFailedMessage := '';
  GDriverStoreMayRemain := False;
  GWmsPrereqOk := False;

  if VirtualMidiGateRequired then
  begin
    if not VirtualMidiPresent then
    begin
      MsgBox(VirtualMidiFixPath, mbError, MB_OK);
      Result := False;
    end;
  end
  else
  begin
    { Win11 WMS flavor: refuse non-Win11 and missing midisrv before copying files. }
    if not IsWindows11OrNewer then
    begin
      MsgBox(WmsOsRequired, mbError, MB_OK);
      Result := False;
      Exit;
    end;
    if not WmsServicePresent then
    begin
      MsgBox(WmsUnavailable + #13#10#13#10 + EmptyPortsNotSuccess, mbError, MB_OK);
      Result := False;
      Exit;
    end;
    GWmsPrereqOk := True;
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
    Log('WinUSB association failed: could not start pnputil.');
    GGatesFailedMessage :=
      'Could not associate the MT4 with WinUSB (driver tool failed to start).';
    Result := False;
    Exit;
  end;

  { 0 = success; 3010 = reboot required after driver package install. }
  if (ResultCode <> 0) and (ResultCode <> ERROR_SUCCESS_REBOOT_REQUIRED) then
  begin
    { Keep UI short — FinishedLabel truncates long contributor prose. Detail stays in Setup log. }
    Log(
      'WinUSB association failed (pnputil exit ' + IntToStr(ResultCode) + '). ' +
      'Clean machines often reject an unsigned INF / missing catalog. ' +
      'Lab: installer/sign-lab-package.ps1. Public policy: docs/dev/authenticode-and-smartscreen.md.');
    GGatesFailedMessage :=
      'Could not associate the MT4 with WinUSB.' + #13#10 +
      'Windows may reject an unsigned driver package on a clean PC.' + #13#10 +
      'Program files were not left as a successful install. See the user guide (WinUSB / trust notes), then retry.';
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
  { Must write HKCU / Task Scheduler for the interactive user — not the elevated admin profile.
    Pass --midi-backend so Auto-Start matches this Setup flavor (WMS vs virtualMIDI). }
  WizardForm.StatusLabel.Caption := 'Registering Auto-Start for your user session…';
  WizardForm.StatusLabel.Update;

  if not ExecAsOriginalUser(
      ExpandConstant('{app}\{#MyAppExeName}'),
      '{#BridgeRegisterParams}',
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
    Log(
      'Auto-Start registration failed (Bridge exit ' + IntToStr(ResultCode) + '). ' +
      'Expected Task Scheduler or HKCU Run with --auto-session and flavor midi-backend.');
    GGatesFailedMessage :=
      'Could not register Auto-Start (Bridge reported an error).' + #13#10 +
      'Program files were not left as a successful install. Retry Setup, or see the user guide Auto-Start section.';
    Result := False;
    Exit;
  end;

  Result := True;
end;

function AutoStartTaskPresent: Boolean;
var
  ResultCode: Integer;
begin
  Result :=
    Exec(
      ExpandConstant('{cmd}'),
      '/C schtasks /Query /TN "UnitorMt4BridgeAutoStart" >NUL 2>&1',
      '',
      SW_HIDE,
      ewWaitUntilTerminated,
      ResultCode) and (ResultCode = 0);
end;

function UnregisterAutoStartBestEffort: Boolean;
var
  ResultCode: Integer;
begin
  Result := False;
  if not FileExists(ExpandConstant('{app}\{#MyAppExeName}')) then
    Exit;
  Log('Best-effort Auto-Start unregister before Abort…');
  if ExecAsOriginalUser(
      ExpandConstant('{app}\{#MyAppExeName}'),
      '--unregister-auto-start',
      ExpandConstant('{app}'),
      SW_HIDE,
      ewWaitUntilTerminated,
      ResultCode) then
    Result := (ResultCode = 0)
  else
    Result := False;
  Log('Unregister Auto-Start best-effort exit=' + IntToStr(ResultCode));
end;

function RegisterAutoStartBestEffort: Boolean;
var
  ResultCode: Integer;
begin
  Result := False;
  if not FileExists(ExpandConstant('{app}\{#MyAppExeName}')) then
    Exit;
  Log('Best-effort Auto-Start re-register (upgrade restore)…');
  if ExecAsOriginalUser(
      ExpandConstant('{app}\{#MyAppExeName}'),
      '{#BridgeRegisterParams}',
      ExpandConstant('{app}'),
      SW_HIDE,
      ewWaitUntilTerminated,
      ResultCode) then
    Result := (ResultCode = 0)
  else
    Result := False;
  Log('Re-register Auto-Start best-effort exit=' + IntToStr(ResultCode));
end;

procedure AbortFailedGates;
begin
  { If we registered Auto-Start this run, clear it before Inno rolls back files. }
  if GAutoStartOk then
  begin
    UnregisterAutoStartBestEffort;
    GAutoStartOk := False;
  end
  else if GHadAutoStartBefore then
  begin
    { Upgrade path: a failed register may have cleared a working entry — try restore. }
    RegisterAutoStartBestEffort;
  end;

  if GWinUsbOk then
    GDriverStoreMayRemain := True;

  MsgBox(GGatesFailedMessage, mbError, MB_OK);
  Abort;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    GHadAutoStartBefore := AutoStartTaskPresent;
    GWinUsbOk := BindMt4WinUsb;
    if not GWinUsbOk then
    begin
      { Abort rolls back copied files / ARP so a failed gate is not left as installed. }
      AbortFailedGates;
    end;

    GAutoStartOk := RegisterAutoStartAsUser;
    if not GAutoStartOk then
    begin
      AbortFailedGates;
    end;

    if VirtualMidiGateRequired and (not VirtualMidiPresent) then
    begin
      GGatesFailedMessage := VirtualMidiFixPath;
      AbortFailedGates;
    end;

    if (not VirtualMidiGateRequired) and (not GWmsPrereqOk) then
    begin
      GGatesFailedMessage := WmsUnavailable + #13#10#13#10 + EmptyPortsNotSuccess;
      AbortFailedGates;
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
      if VirtualMidiGateRequired then
        SuccessBody :=
          'Unitor MT4 Bridge is installed under Program Files.' + #13#10#13#10 +
          'virtualMIDI is present; WinUSB association and Auto-Start registration both reported success.' + #13#10 +
          'After sign-in (or when you plug in the MT4), virtual MIDI ports should appear without a manual Bridge launch.' + #13#10#13#10 +
          'Daily use does not require Administrator.' + #13#10#13#10 +
          'Getting started: https://github.com/tensquaresoftware/unitor-win64-driver/blob/main/docs/user/README.md'
      else
        SuccessBody :=
          'Unitor MT4 Bridge is installed under Program Files.' + #13#10#13#10 +
          WmsPathHonesty + #13#10 +
          'WinUSB association and Auto-Start registration both reported success.' + #13#10 +
          'After sign-in (or when you plug in the MT4), MIDI ports should appear via Windows MIDI Services.' + #13#10#13#10 +
          EmptyPortsNotSuccess + #13#10#13#10 +
          'Daily use does not require Administrator.' + #13#10#13#10 +
          'Getting started: https://github.com/tensquaresoftware/unitor-win64-driver/blob/main/docs/user/README.md';
      if GRebootRecommended then
        SuccessBody := SuccessBody + #13#10#13#10 +
          'Windows reported that a reboot is recommended to finish driver installation. Reboot before relying on WinUSB.';
      WizardForm.FinishedLabel.Caption := SuccessBody;
    end
    else
    begin
      { Keep under FinishedLabel capacity — MsgBox already showed the failure detail. }
      WizardForm.FinishedHeadingLabel.Caption := 'Installation incomplete';
      if GGatesFailedMessage <> '' then
        WizardForm.FinishedLabel.Caption :=
          GGatesFailedMessage + #13#10#13#10 +
          'Bridge program files were not left as a successful install.'
      else
        WizardForm.FinishedLabel.Caption :=
          'Setup could not finish. Bridge program files were not left as a successful install.';
      if GDriverStoreMayRemain then
        WizardForm.FinishedLabel.Caption :=
          WizardForm.FinishedLabel.Caption + #13#10 +
          'A WinUSB driver package may still remain in Windows Driver Store.';
      WizardForm.FinishedLabel.Caption :=
        WizardForm.FinishedLabel.Caption + #13#10 +
        'Fix the issue, then run Setup again.';
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
            'Remove any leftover Unitor MT4 Bridge Auto-Start entry manually if it remains.' + #13#10 +
            'Other Windows user accounts on this PC are not cleared by this uninstall — unregister while signed in as each user.' + #13#10 +
            'The MT4 WinUSB association may remain in Driver Store (normal).',
            mbError,
            MB_OK);
      end;
    end
    else
    begin
      MsgBox(
        'Could not verify Auto-Start unregister (exit marker missing).' + #13#10 +
        'If Bridge still starts at logon, remove the Unitor MT4 Bridge Auto-Start entry manually.' + #13#10 +
        'Other Windows accounts are not cleared by this uninstall.' + #13#10 +
        'The MT4 WinUSB association may remain in Driver Store (normal).',
        mbInformation,
        MB_OK);
    end;
  end;
end;
