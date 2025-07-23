
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
WizardImageFile=assets\dualsensitive_background_240x459_blackfill.bmp
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
  DisclaimerCheckBox: TNewCheckBox;
  //DisclaimerPage: TInputQueryWizardPage;
  DisclaimerAccepted: Boolean;
  DisclaimerPage: TWizardPage;
  //DisclaimerAccepted: TNewCheckBox;

procedure CreateDisclaimerPage();
var
  Memo: TMemo;
begin
  DisclaimerAccepted := False;
  DisclaimerPage := CreateCustomPage(
    wpWelcome,
    'Disclaimer',
    'Please read and accept the following disclaimer before continuing.'
  );

  Memo := TMemo.Create(DisclaimerPage);
  Memo.Parent := DisclaimerPage.Surface;
  Memo.Left := ScaleX(0);
  Memo.Top := ScaleY(0);
  Memo.Width := DisclaimerPage.Surface.Width;  // Full width
  Memo.Height := ScaleY(150);  // Adjust height as needed
  Memo.ReadOnly := True;
  Memo.ScrollBars := ssVertical;
  Memo.WordWrap := True;
  Memo.Text :=
'This mod is provided "as is" with no warranty or guarantee of performance.' + #13#10 +
'By continuing, you acknowledge that you are installing third-party software' + #13#10 +
'which may modify or interact with the game in ways not intended by its original developers.' + #13#10 +
'' + #13#10 +
'Use at your own risk. The authors and platforms are not responsible' + #13#10 +
'for any damage, data loss, or other issues caused by this software.' + #13#10 +
'' + #13#10 +
'This is a non-commercial fan-made project. All rights to the game "Control"' + #13#10 +
'and its characters belong to Remedy Entertainment and 505 Games.' + #13#10 +
'Artwork inspired by the character Jessie Faden.' + #13#10 +
'' + #13#10 +
'Created by Thanos Petsas - https://thanasispetsas.com';

  // Create and position the checkbox under the memo
  DisclaimerCheckBox := TNewCheckBox.Create(DisclaimerPage);
  DisclaimerCheckBox.Parent := DisclaimerPage.Surface;
  DisclaimerCheckBox.Top := Memo.Top + Memo.Height + ScaleY(8);
  DisclaimerCheckBox.Left := ScaleX(0);
  DisclaimerCheckBox.Width := DisclaimerPage.Surface.Width;
  DisclaimerCheckBox.Height := ScaleY(20);
  DisclaimerCheckBox.Caption := 'I have read and accept the disclaimer above.';
  //DisclaimerCheckBox.OnClick := @CurPageChangedCheck;

  WizardForm.NextButton.Enabled := False;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := False;
  if Assigned(DisclaimerPage) and (PageID = DisclaimerPage.ID) then
    Result := DisclaimerAccepted;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if Assigned(DisclaimerPage) and (CurPageID = DisclaimerPage.ID) then
    WizardForm.NextButton.Enabled := DisclaimerAccepted;
end;

procedure CurPageChangedCheck(Sender: TObject);
begin
  DisclaimerAccepted := TNewCheckBox(Sender).Checked;
  WizardForm.NextButton.Enabled := DisclaimerAccepted;
end;


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
    if not DisclaimerAccepted then
    begin
      MsgBox('You must accept the disclaimer to continue.', mbError, MB_OK);
      Result := False;
    end;

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

function ExtractJsonValue(Line: string): string;
var
  i: Integer;
begin
  Result := '';
  i := Pos(':', Line);
  if i > 0 then
  begin
    Result := Trim(Copy(Line, i + 1, Length(Line)));
    // Remove surrounding quotes and trailing commas
    if (Length(Result) >= 2) and (Result[1] = '"') then
    begin
      Delete(Result, 1, 1);
      i := Pos('"', Result);
      if i > 0 then
        Result := Copy(Result, 1, i - 1);
    end;
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
                    // EpicInstallPath := StripQuotesAndKeyPrefix(Lines[i], '');
                    EpicInstallPath := ExtractJsonValue(Lines[i]);
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

var
  DeleteLogsPage: TWizardPage;
  DeleteLogsCheckbox: TNewCheckBox;
  LogPaths: TStringList;

procedure CheckAndAddPath(BasePath: string);
var
  DualSensitiveDir: string;
begin
DualSensitiveDir := BasePath + '\plugins\DualSensitive';
if FileExists(DualSensitiveDir + '\dualsensitive-service.log') or
   FileExists(DualSensitiveDir + '\dualsensitive-client.log') then
  LogPaths.Add(DualSensitiveDir);
end;

procedure DetectLogFiles();
var
  SteamPath, EpicPath, ManualPath: string;

begin
  if RegQueryStringValue(HKCU, 'Software\Valve\Steam', 'SteamPath', SteamPath) then
    CheckAndAddPath(SteamPath + '\steamapps\common\Control');

  if EpicInstallPath <> '' then
    CheckAndAddPath(EpicInstallPath);

  CheckAndAddPath(ExpandConstant('{app}'));
end;

procedure CreateLogDeletePrompt();
var
  answer: Integer;
begin
  if LogPaths.Count = 0 then Exit;

  answer := MsgBox(
    'Log files from DualSensitive were found in one or more Control installations.' + #13#10#13#10 +
    'Do you also want to delete these log folders (including Steam/Epic paths)?',
    mbConfirmation, MB_YESNO);

  if answer = IDYES then
  begin
    DeleteLogsCheckbox := TNewCheckBox.Create(nil); // simulate user consent
    DeleteLogsCheckbox.Checked := True;
  end;
end;

procedure InitializeUninstallProgressForm();
begin
  LogPaths := TStringList.Create;
  DetectLogFiles();
  if LogPaths.Count > 0 then
  begin
    if MsgBox(
         'Log files from DualSensitive were found in one or more Control installations.' + #13#10#13#10 +
         'Do you want to delete these log folders (including Steam/Epic paths)?',
         mbConfirmation, MB_YESNO) = IDYES
    then
    begin
      DeleteLogsCheckbox := TNewCheckBox.Create(nil); // simulate consent
      DeleteLogsCheckbox.Checked := True;
    end;
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  i: Integer;
begin
  if (CurUninstallStep = usPostUninstall) and
     (LogPaths <> nil) and
     (DeleteLogsCheckbox <> nil) and
     DeleteLogsCheckbox.Checked then
  begin
    for i := 0 to LogPaths.Count - 1 do
    begin
      Log('Deleting: ' + LogPaths[i]);
      DelTree(LogPaths[i], True, True, True);
    end;
  end;
end;

procedure OnVisitWebsiteClick(Sender: TObject);
var
    ErrCode: integer;
begin
  ShellExec('open', 'https://www.dualsensitive.com/', '', '', SW_SHOW, ewNoWait, ErrCode);
end;

procedure InitializeWizard;
var
  InfoLabel1, InfoLabel2, InfoLabel3, InfoLabel4: TLabel;
  IsSteamInstalled, IsEpicInstalled: Boolean;
  CurrentTop: Integer;
  ThankYouLabel, WebsiteLabel: TNewStaticText;
begin
  CreateDisclaimerPage();
  DisclaimerCheckBox.OnClick := @CurPageChangedCheck;
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

  CurrentTop := InfoLabel4.Top + ScaleY(24);

  // Steam checkbox
  if IsSteamInstalled then
  begin
    SteamCheckbox := TCheckBox.Create(WizardForm);
    SteamCheckbox.Parent := MyPage.Surface;
    SteamCheckbox.Top := CurrentTop;
    SteamCheckbox.Left := ScaleX(0);
    SteamCheckbox.Width := ScaleX(300);
    SteamCheckbox.Height := ScaleY(20);
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
    EpicCheckbox.Height := ScaleY(20);
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
  ManualCheckbox.Height := ScaleY(20);
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

  begin
    // Thank-you message (non-clickable)
    ThankYouLabel := TNewStaticText.Create(WizardForm);
    ThankYouLabel.Parent := WizardForm.FinishedPage;
    ThankYouLabel.Caption := #13#10 + 'Thank you for installing the Control — DualSensitive Mod!' + #13#10 +
                             'For news and updates, please visit:';
    ThankYouLabel.Top := WizardForm.FinishedLabel.Top + WizardForm.FinishedLabel.Height + ScaleY(16);
    ThankYouLabel.Left := WizardForm.FinishedLabel.Left;
    ThankYouLabel.AutoSize := True;

    // Hyperlink (clickable)
    WebsiteLabel := TNewStaticText.Create(WizardForm);
    WebsiteLabel.Parent := WizardForm.FinishedPage;
    WebsiteLabel.Caption := 'https://www.dualsensitive.com/';
    WebsiteLabel.Font.Color := clBlue;
    WebsiteLabel.Font.Style := WebsiteLabel.Font.Style + [fsUnderline];
    WebsiteLabel.Cursor := crHand;
    WebsiteLabel.OnClick := @OnVisitWebsiteClick;
    WebsiteLabel.Top := ThankYouLabel.Top + ThankYouLabel.Height + ScaleY(8);
    WebsiteLabel.Left := ThankYouLabel.Left;
    WebsiteLabel.AutoSize := True;
  end;
end;
