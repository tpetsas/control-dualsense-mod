
; ControlDSModInstaller.iss
; Updated Installer script for Control - DualSensitive Mod

#define ModName "Control — DualSensitive Mod"
#define ModExeName "dualsensitive-service.exe"
#define VbsScript "launch-service.vbs"
#define INIFile "dualsense-mod.ini"
#define PluginDLL "dualsense-mod.dll"
#define ProxyDLL "xinput1_4.dll"
#define AppId "Control.DualSensitive.Mod"

[Setup]
AppId={#AppId}
AppName={#ModName}
AppVersion=2.2
DefaultDirName={pf}\DualSensitive\{#ModName}
DefaultGroupName={#ModName}
OutputDir=.
OutputBaseFilename=Control-DualSensitive-Mod_Setup
Compression=lzma
SolidCompression=yes
SetupIconFile=assets\control_installer.ico
WizardSmallImageFile=assets\DualSensitive_dark.bmp
WizardImageFile=assets\inno_side_background_164x314.bmp
UninstallDisplayIcon={app}\control_uninstaller.ico
DisableProgramGroupPage=yes

[CustomMessages]
InstallInfoLine1=Install Control — DualSensitive Mod for one or more game versions below.
InstallInfoLine2=You can select Steam, Epic, or a custom installation path. Leave unchecked any
InstallInfoLine3=version you don't want to mod.

;InstallInfoLine1=Setup will install Control - DualSensitive Mod into the following folder.
;InstallInfoLine2=The components to be installed include: the Control Plugin Loader (xinput1_4.dll),
;InstallInfoLine3=the dualsense mod (dualsense-mod.dll), and the dualsensitive service.
InstallInfoLine4=To continue, click Next. If you would like to select a different directory, click Browse.

//InstallInfoLine2=The main install directory is used only for uninstall-related files.

[Files]
Source: "files\{#ProxyDLL}"; DestDir: "{code:GetInstallPath}"; Flags: ignoreversion
Source: "files\{#PluginDLL}"; DestDir: "{code:GetInstallPath}\plugins"; Flags: ignoreversion
Source: "files\{#INIFile}"; DestDir: "{code:GetInstallPath}\plugins"; Flags: ignoreversion
Source: "files\{#ModExeName}"; DestDir: "{code:GetInstallPath}\plugins\DualSensitive"; Flags: ignoreversion
Source: "files\{#VbsScript}"; DestDir: "{code:GetInstallPath}\plugins\DualSensitive"; Flags: ignoreversion
Source: "assets\control_uninstaller.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "assets\DualSensitive_dark.png"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Uninstall {#ModName}"; Filename: "{uninstallexe}"

[Run]
Filename: "schtasks.exe"; \
  Parameters: "{code:GetSchedTaskCommand}"; \
  Flags: runhidden

;Filename: "schtasks.exe"; \
;Parameters: "/Create /TN ""DualSensitiveService"" \
;  /TR ""cmd.exe /c cd /d ^""{code:GetInstallPath}\\plugins^"" && ^""{#ModExeName}^"""" \
;  /SC ONCE /ST 00:00 /RL HIGHEST /F"; \
;Flags: runhidden


;Filename: "schtasks.exe"; \
;  Parameters: "/Create /TN ""DualSensitiveService"" /TR ""cmd.exe /c cd /d ^""{code:GetInstallPath}\\plugins^"" && ^""{#ModExeName}^"" "" /SC ONLOGON /RL HIGHEST /F"; \
;  Flags: runhidden


;Filename: "schtasks.exe"; \
;  Parameters: "/Delete /TN ""DualSensitiveService"" /F"; \
;  Flags: runhidden runascurrentuser; \
;  StatusMsg: "Cleaning up old service tasks..."

[UninstallDelete]
Type: files; Name: "{code:GetInstallPath}\{#ProxyDLL}"
Type: files; Name: "{code:GetInstallPath}\plugins\{#ModExeName}"
Type: files; Name: "{code:GetInstallPath}\plugins\{#PluginDLL}"
Type: dirifempty; Name: "{code:GetInstallPath}\plugins"
Type: dirifempty; Name: "{app}"

[Code]

var
  SteamCheckbox, EpicCheckbox, ManualCheckbox: TCheckBox;
  ManualPathEdit: TEdit;
  ManualBrowseButton: TButton;
  EpicInstallPath: string;
  SelectedInstallPath: string;
  MyPage: TWizardPage;

procedure BrowseManualPath(Sender: TObject);
var Dir: string;
begin
  Dir := ManualPathEdit.Text;
  if BrowseForFolder('Select game folder...', Dir, false) then
    ManualPathEdit.Text := Dir;
end;

procedure ManualCheckboxClick(Sender: TObject);
var
  Enabled: Boolean;
begin
  Enabled := ManualCheckbox.Checked;
  ManualPathEdit.Enabled := Enabled;
  ManualBrowseButton.Enabled := Enabled;
end;

function GetInstallPath(Default: string): string;
begin
  //Result := ExpandConstant('{app}')
  Result := SelectedInstallPath;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  // Optional debug
  Log('Page changed: ' + IntToStr(CurPageID));
end;

function GetSchedTaskCommand(Param: string): string;
var
  vbsPath, exePath, fullCmd: string;
begin
  // Paths to launch-service.vbs and dualsensitive-service.exe
  vbsPath := GetInstallPath('') + '\plugins\DualSensitive\launch-service.vbs';
  exePath := GetInstallPath('') + '\plugins\DualSensitive\dualsensitive-service.exe';

  // Escape quotes for schtasks + Inno
  fullCmd :=
    '/Create /TN "DualSensitive Service" ' +
    '/TR "wscript.exe \"' + vbsPath + '\" \"' + exePath + '\"" ' +
    '/SC ONCE /ST 00:00 /RL HIGHEST /F';

  Log('Scheduled Task Command: ' + fullCmd);
  // Optional: Show dialog during install for debug
  // MsgBox('Scheduled Task Command:'#13#10 + fullCmd, mbInformation, MB_OK);

  Result := fullCmd;
end;


function NextButtonClick(CurPageID: Integer): Boolean;
var
  SteamPath: string;
begin
  Result := True;

  if CurPageID = MyPage.ID then
  begin
    if SteamCheckbox <> nil then
begin
  if SteamCheckbox.Checked then
  begin
    if RegQueryStringValue(HKCU, 'Software\Valve\Steam', 'SteamPath', SteamPath) then
      SelectedInstallPath := SteamPath + '\steamapps\common\Control'
    else
      SelectedInstallPath := ''; // fallback
    Log('Using Steam path: ' + SelectedInstallPath);
  end;
end;

if EpicCheckbox <> nil then
begin
  if EpicCheckbox.Checked then
  begin
    SelectedInstallPath := EpicInstallPath;
    Log('Using Epic path: ' + SelectedInstallPath);
  end;
end;
    if ManualCheckbox.Checked then
    begin
      SelectedInstallPath := ManualPathEdit.Text;
      Log('Using manual path: ' + SelectedInstallPath);
    end;

    if not DirExists(SelectedInstallPath) then
    begin
      if not CreateDir(SelectedInstallPath) then
      begin
        MsgBox('Failed to create folder: ' + SelectedInstallPath, mbError, MB_OK);
        Result := False;
        exit;
       end;
    end;

    Log('SelectedInstallPath: ' + SelectedInstallPath);

  end;
end;
(*
procedure InitializeWizard;
var
  InfoLabel1, InfoLabel2, InfoLabel3, InfoLabel4: TLabel;
begin
  // Create the custom page to appear *after* the Welcome page
  MyPage := CreateCustomPage(wpSelectDir, 'Choose Game Versions', 'Select which game versions to install the mod for.');

  // Info labels
  InfoLabel1 := TLabel.Create(WizardForm);
  InfoLabel1.Parent := MyPage.Surface;
  InfoLabel1.Top := ScaleY(0);
  InfoLabel1.Left := ScaleX(0);
  InfoLabel1.Font.Style := [fsBold];
  InfoLabel1.Caption := CustomMessage('InstallInfoLine1');

  InfoLabel2 := TLabel.Create(WizardForm);
  InfoLabel2.Parent := MyPage.Surface;
  InfoLabel2.Top := InfoLabel1.Top + ScaleY(20);
  InfoLabel2.Left := ScaleX(0);
  InfoLabel2.Caption := CustomMessage('InstallInfoLine2');

  InfoLabel3 := TLabel.Create(WizardForm);
  InfoLabel3.Parent := MyPage.Surface;
  InfoLabel3.Top := InfoLabel2.Top + ScaleY(20);
  InfoLabel3.Left := ScaleX(0);
  InfoLabel3.Caption := CustomMessage('InstallInfoLine3');

  InfoLabel4 := TLabel.Create(WizardForm);
  InfoLabel4.Parent := MyPage.Surface;
  InfoLabel4.Top := InfoLabel3.Top + ScaleY(30);
  InfoLabel4.Left := ScaleX(0);
  InfoLabel4.Caption := CustomMessage('InstallInfoLine4');

  // Steam checkbox
  SteamCheckbox := TCheckBox.Create(WizardForm);
  SteamCheckbox.Parent := MyPage.Surface;
  SteamCheckbox.Top := InfoLabel4.Top + ScaleY(40);
  SteamCheckbox.Left := ScaleX(0);
  SteamCheckbox.Width := ScaleX(300);
  SteamCheckbox.Caption := 'Install for Steam (Control)';

  // Epic checkbox
  EpicCheckbox := TCheckBox.Create(WizardForm);
  EpicCheckbox.Parent := MyPage.Surface;
  EpicCheckbox.Top := SteamCheckbox.Top + ScaleY(24);
  EpicCheckbox.Left := ScaleX(0);
  EpicCheckbox.Width := ScaleX(300);
  EpicCheckbox.Caption := 'Install for Epic Games (Control)';

  // Manual checkbox
  ManualCheckbox := TCheckBox.Create(WizardForm);
  ManualCheckbox.Parent := MyPage.Surface;
  ManualCheckbox.Top := EpicCheckbox.Top + ScaleY(24);
  ManualCheckbox.Left := ScaleX(0);
  ManualCheckbox.Width := ScaleX(300);
  ManualCheckbox.Caption := 'Install to custom path:';

  // Manual path edit box
  ManualPathEdit := TEdit.Create(WizardForm);
  ManualPathEdit.Parent := MyPage.Surface;
  ManualPathEdit.Top := ManualCheckbox.Top + ScaleY(24);
  ManualPathEdit.Left := ScaleX(0);
  ManualPathEdit.Width := ScaleX(300);
  ManualPathEdit.Text := 'C:\Games\Control';

  // Browse button
  ManualBrowseButton := TButton.Create(WizardForm);
  ManualBrowseButton.Parent := MyPage.Surface;
  ManualBrowseButton.Top := ManualPathEdit.Top;
  ManualBrowseButton.Left := ManualPathEdit.Left + ManualPathEdit.Width + ScaleX(8);
  ManualBrowseButton.Width := ScaleX(75);
  ManualBrowseButton.Caption := 'Browse...';
  ManualBrowseButton.OnClick := @BrowseManualPath;
end;
*)

function FileExistsInSteam(): Boolean;
var
  SteamPath, VdfPath, GamePath: string;
begin
  Result := False;
  if RegQueryStringValue(HKCU, 'Software\Valve\Steam', 'SteamPath', SteamPath) then
  begin
    GamePath := SteamPath + '\steamapps\common\Control\Control.exe';
    if FileExists(GamePath) then
      Result := True;
  end;
end;

function StripQuotesAndKeyPrefix(S, Key: string): string;
var
  i: Integer;
begin
  Result := Trim(S);
  // Remove "Key:" prefix
  if Pos(Key, Result) > 0 then
    Delete(Result, 1, Length(Key));
  Result := Trim(Result);

  // Remove all quotation marks manually
  i := 1;
  while i <= Length(Result) do
  begin
    if Result[i] = '"' then
      Delete(Result, i, 1)
    else
      i := i + 1;
  end;
end;

function FileExistsInEpic(): Boolean;
var
  FindRec: TFindRec;
  ManifestDir, FilePath, Line: string;
  Lines: TArrayOfString;
  i: Integer;
begin
  Result := False;
  ManifestDir := 'C:\ProgramData\Epic\EpicGamesLauncher\Data\Manifests';

  Log('Checking if the game is installed via Epic');
  if DirExists(ManifestDir) then
  begin
    if FindFirst(ManifestDir + '\*.item', FindRec) then
    begin
      try
        repeat
          FilePath := ManifestDir + '\' + FindRec.Name;
          if LoadStringsFromFile(FilePath, Lines) then
          begin
            for i := 0 to GetArrayLength(Lines) - 1 do
            begin
              Line := Lines[i];
              if Pos(Uppercase('"DisplayName": "Control"'), Uppercase(Line)) > 0 then
              begin
                // Found the Control manifest
                Log('Found the Epic Manifest of Control: ' + FilePath);
                // Now look forward for "InstallLocation"
                while i < GetArrayLength(Lines) - 1 do
                begin
                  i := i + 1;
                  if Pos('"InstallLocation":', Lines[i]) > 0 then
                  begin
                    // Extract value from: "InstallLocation":"C:\\Path\\To\\Control"
                    EpicInstallPath := StripQuotesAndKeyPrefix(Lines[i], 'InstallLocation:');
                    if EpicInstallPath = '' then
                        Log('Warning: Epic path was found but empty after parsing');
                    break;
                  end;
                end;
                Result := True;
                exit;
              end;
            end;
          end;
        until not FindNext(FindRec);
      finally
        FindClose(FindRec);
      end;
    end;
  end;
end;



procedure InitializeWizard;
var
  InfoLabel1, InfoLabel2, InfoLabel3, InfoLabel4: TLabel;
  IsSteamInstalled, IsEpicInstalled: Boolean;
  CurrentTop: Integer;
begin
  MyPage := CreateCustomPage(wpSelectDir, 'Choose Game Versions', 'Select which game versions to install the mod for.');

  IsSteamInstalled := FileExistsInSteam();
  IsEpicInstalled := FileExistsInEpic();

  // Info labels
  InfoLabel1 := TLabel.Create(WizardForm);
  InfoLabel1.Parent := MyPage.Surface;
  InfoLabel1.Top := ScaleY(0);
  InfoLabel1.Left := ScaleX(0);
  InfoLabel1.Font.Style := [fsBold];
  InfoLabel1.Caption := CustomMessage('InstallInfoLine1');

  InfoLabel2 := TLabel.Create(WizardForm);
  InfoLabel2.Parent := MyPage.Surface;
  InfoLabel2.Top := InfoLabel1.Top + ScaleY(20);
  InfoLabel2.Left := ScaleX(0);
  InfoLabel2.Caption := CustomMessage('InstallInfoLine2');

  InfoLabel3 := TLabel.Create(WizardForm);
  InfoLabel3.Parent := MyPage.Surface;
  InfoLabel3.Top := InfoLabel2.Top + ScaleY(20);
  InfoLabel3.Left := ScaleX(0);
  InfoLabel3.Caption := CustomMessage('InstallInfoLine3');

  InfoLabel4 := TLabel.Create(WizardForm);
  InfoLabel4.Parent := MyPage.Surface;
  InfoLabel4.Top := InfoLabel3.Top + ScaleY(30);
  InfoLabel4.Left := ScaleX(0);
  InfoLabel4.Caption := CustomMessage('InstallInfoLine4');

  // ✅ Fix: start layout below InfoLabel4
  CurrentTop := InfoLabel4.Top + ScaleY(24);

  // Steam checkbox
  if IsSteamInstalled then
  begin
    SteamCheckbox := TCheckBox.Create(WizardForm);
    SteamCheckbox.Parent := MyPage.Surface;
    SteamCheckbox.Top := CurrentTop;
    SteamCheckbox.Left := ScaleX(0);
    SteamCheckbox.Width := ScaleX(300);
    SteamCheckbox.Caption := 'Install for Steam';
    SteamCheckbox.Checked := True;
    CurrentTop := CurrentTop + ScaleY(24);
  end;

  // Epic checkbox
  if IsEpicInstalled then
  begin
    EpicCheckbox := TCheckBox.Create(WizardForm);
    EpicCheckbox.Parent := MyPage.Surface;
    EpicCheckbox.Top := CurrentTop;
    EpicCheckbox.Left := ScaleX(0);
    EpicCheckbox.Width := ScaleX(300);
    EpicCheckbox.Caption := 'Install for Epic Games';
    if not IsSteamInstalled then
      EpicCheckbox.Checked := True;
    CurrentTop := CurrentTop + ScaleY(24);
  end;

  // Manual path checkbox
  ManualCheckbox := TCheckBox.Create(WizardForm);
  ManualCheckbox.Parent := MyPage.Surface;
  ManualCheckbox.Top := CurrentTop;
  ManualCheckbox.Left := ScaleX(0);
  ManualCheckbox.Width := ScaleX(300);
  ManualCheckbox.Caption := 'Install to custom path:';
  ManualCheckbox.OnClick := @ManualCheckboxClick;
  ManualCheckbox.Checked := (not IsSteamInstalled) and (not IsEpicInstalled);

  CurrentTop := CurrentTop + ScaleY(24);

  // Manual path edit
  ManualPathEdit := TEdit.Create(WizardForm);
  ManualPathEdit.Parent := MyPage.Surface;
  ManualPathEdit.Top := CurrentTop;
  ManualPathEdit.Left := ScaleX(0);
  ManualPathEdit.Width := ScaleX(300);
  ManualPathEdit.Text := 'C:\Games\Control';

  // Browse button
  ManualBrowseButton := TButton.Create(WizardForm);
  ManualBrowseButton.Parent := MyPage.Surface;
  ManualBrowseButton.Top := CurrentTop;
  ManualBrowseButton.Left := ManualPathEdit.Left + ManualPathEdit.Width + ScaleX(8);
  ManualBrowseButton.Width := ScaleX(75);
  ManualBrowseButton.Caption := 'Browse...';
  ManualBrowseButton.OnClick := @BrowseManualPath;

  // Initialize enabled/disabled state
  ManualCheckboxClick(nil);

end;

