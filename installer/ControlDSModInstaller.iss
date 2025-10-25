
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
VersionInfoVersion=2.2.2.0
VersionInfoTextVersion=2.2.2.0
VersionInfoProductVersion=2.2.2.0
DefaultDirName={autopf}\DualSensitive\{#ModName}
DefaultGroupName={#ModName}
OutputDir=.
OutputBaseFilename=Control-DualSensitive-Mod_Setup
Compression=lzma
SolidCompression=yes
SetupIconFile=assets\control_installer.ico
WizardSmallImageFile=assets\DualSensitive_dark.bmp
WizardImageFile=assets\dualsensitive_background_240x459_blackfill.bmp
UninstallDisplayIcon={app}\control_uninstaller.ico
UninstallDisplayName={#ModName}
DisableProgramGroupPage=yes

[CustomMessages]
InstallInfoLine1=Install Control — DualSensitive Mod for one or more game versions below.
InstallInfoLine2=You can select Steam, Epic, or a custom installation path. Leave unchecked any
InstallInfoLine3=version you don't want to mod.
InstallInfoLine4=To continue, click Next. If you would like to select a different directory, click Browse.

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
  DisclaimerAccepted: Boolean;
  DisclaimerPage: TWizardPage;
  SteamCheckbox, EpicCheckbox, ManualCheckbox: TCheckBox;
  ManualPathEdit: TEdit;
  ManualBrowseButton: TButton;
  EpicInstallPath: string;
  SelectedInstallPath: string;
  MyPage: TWizardPage;
  FinishExtrasCreated: Boolean;
  SteamInstallPath: string;


procedure SafeSetParent(Control: TControl; ParentCtrl: TWinControl);
begin
  // Always set Parent as the first thing after create
  Control.Parent := ParentCtrl;
end;

// Detect Windows 11+ by build number (Win11 starts at build 22000)
function IsWindows11OrNewer: Boolean;
var
  s: string;
  build: Integer;
begin
  // Read build number; Win11 starts at build 22000
  s := '';
  if not RegQueryStringValue(HKLM,
    'SOFTWARE\Microsoft\Windows NT\CurrentVersion', 'CurrentBuildNumber', s) then
  begin
    RegQueryStringValue(HKLM,
      'SOFTWARE\Microsoft\Windows NT\CurrentVersion', 'CurrentBuild', s);
  end;
  build := StrToIntDef(s, 0);
  Result := (build >= 22000);
end;

procedure CurPageChangedCheck(Sender: TObject);
begin
  DisclaimerAccepted := TNewCheckBox(Sender).Checked;
  if not WizardSilent and Assigned(WizardForm.NextButton) then
    WizardForm.NextButton.Enabled := DisclaimerAccepted;
end;

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

  // Owner MUST be WizardForm; parent is the page surface
  Memo := TMemo.Create(WizardForm);                // was: TMemo.Create(DisclaimerPage)
  Memo.Parent := DisclaimerPage.Surface;
  Memo.Left := ScaleX(0);
  Memo.Top := ScaleY(0);
  Memo.Width := DisclaimerPage.Surface.Width;
  Memo.Height := ScaleY(150);
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

  // Same here: owner = WizardForm
  DisclaimerCheckBox := TNewCheckBox.Create(WizardForm);  // was: TNewCheckBox.Create(DisclaimerPage)
  if Assigned(DisclaimerCheckBox) then
  begin
    DisclaimerCheckBox.Parent := DisclaimerPage.Surface;
    DisclaimerCheckBox.Top := Memo.Top + Memo.Height + ScaleY(8);
    DisclaimerCheckBox.Left := ScaleX(0);
    DisclaimerCheckBox.Width := DisclaimerPage.Surface.Width;
    DisclaimerCheckBox.Height := ScaleY(20);
    DisclaimerCheckBox.Caption := 'I have read and accept the disclaimer above.';
    DisclaimerCheckBox.OnClick := @CurPageChangedCheck;
  end;

  if not WizardSilent and Assigned(WizardForm.NextButton) then
    WizardForm.NextButton.Enabled := False;
end;


function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := False;
  if Assigned(DisclaimerPage) and (PageID = DisclaimerPage.ID) then
    Result := DisclaimerAccepted;
end;

procedure OnVisitWebsiteClick(Sender: TObject);
var
  ErrCode: Integer;
begin
  ShellExec('open', 'https://www.dualsensitive.com/', '', '', SW_SHOW, ewNoWait, ErrCode);
end;

procedure CurPageChanged(CurPageID: Integer);
var
  ThankYouLabel, WebsiteLabel: TNewStaticText;
  FS: TFontStyles;
begin
  // keep the disclaimer Next-button gating
  if not WizardSilent and Assigned(WizardForm.NextButton) and Assigned(DisclaimerPage) and
     (CurPageID = DisclaimerPage.ID) then
    WizardForm.NextButton.Enabled := DisclaimerAccepted;

  // Build the Finished-page extras only when that page is shown (Win10-safe)
  if (CurPageID = wpFinished) and (not FinishExtrasCreated) then
  begin
    // Thank-you text
    ThankYouLabel := TNewStaticText.Create(WizardForm);
    ThankYouLabel.Parent := WizardForm.FinishedPage;  // standard page: no .Surface
    ThankYouLabel.Caption := #13#10 +
      'Thank you for installing the Control — DualSensitive Mod!' + #13#10 +
      'For news and updates, please visit:';
    ThankYouLabel.Top := WizardForm.FinishedLabel.Top + WizardForm.FinishedLabel.Height + ScaleY(16);
    ThankYouLabel.Left := WizardForm.FinishedLabel.Left;
    ThankYouLabel.AutoSize := True;

    // Clickable link
    WebsiteLabel := TNewStaticText.Create(WizardForm);
    WebsiteLabel.Parent := WizardForm.FinishedPage;
    WebsiteLabel.Caption := 'https://www.dualsensitive.com/';
    WebsiteLabel.Font.Color := clBlue;
    FS := WebsiteLabel.Font.Style;     // Include requires a variable set
    Include(FS, fsUnderline);
    WebsiteLabel.Font.Style := FS;
    WebsiteLabel.Cursor := crHand;
    WebsiteLabel.OnClick := @OnVisitWebsiteClick;
    WebsiteLabel.Top := ThankYouLabel.Top + ThankYouLabel.Height + ScaleY(8);
    WebsiteLabel.Left := ThankYouLabel.Left;
    WebsiteLabel.AutoSize := True;

    FinishExtrasCreated := True;
  end;
end;

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
if SelectedInstallPath <> '' then
    Result := SelectedInstallPath
else if Assigned(ManualPathEdit) and (ManualPathEdit.Text <> '') then
    Result := ManualPathEdit.Text
else
    // OK to expand these early
    Result := ExpandConstant('{autopf}\DualSensitive\Control — DualSensitive Mod');
end;


function GetSchedTaskCommand(Param: string): string;
var
  vbsPath, exePath: string;
begin
  // Paths to launch-service.vbs and dualsensitive-service.exe
  vbsPath := GetInstallPath('') + '\plugins\DualSensitive\launch-service.vbs';
  exePath := GetInstallPath('') + '\plugins\DualSensitive\dualsensitive-service.exe';

  // If somehow still empty, bail out with a no-op to avoid runtime failure
  if (vbsPath = '\plugins\DualSensitive\launch-service.vbs') or
     (exePath = '\plugins\DualSensitive\dualsensitive-service.exe') then
  begin
    Result := '/Create /TN "DualSensitive Service (invalid path skipped)" /TR "cmd.exe /c exit 0" /SC ONCE /ST 00:00 /RL HIGHEST /F';
    exit;
  end;

  // Escape quotes for schtasks + Inno
  Result :=
    '/Create /TN "DualSensitive Service" ' +
    '/TR "wscript.exe \"' + vbsPath + '\" \"' + exePath + '\"" ' +
    '/SC ONCE /ST 00:00 /RL HIGHEST /F';

  Log('Scheduled Task Command: ' + Result);
  // Optional: Show dialog during install for debug
  // MsgBox('Scheduled Task Command:'#13#10 + fullCmd, mbInformation, MB_OK);
end;

// ---- helpers (TOP-LEVEL, place above FileExistsInSteam) ----

function NormalizeLibraryPath(P: string): string;
var
  i: Integer;
begin
  // strip outer quotes
  if (Length(P) >= 2) and (P[1] = '"') and (P[Length(P)] = '"') then
    P := Copy(P, 2, Length(P) - 2);
  // remove any stray trailing quote
  if (Length(P) > 0) and (P[Length(P)] = '"') then
    Delete(P, Length(P), 1);
  // unescape \\ -> \
  i := 1;
  while i < Length(P) do
    if (P[i] = '\') and (P[i + 1] = '\') then
      Delete(P, i, 1)
    else
      Inc(i);
  // normalize forward slashes
  for i := 1 to Length(P) do
    if P[i] = '/' then P[i] := '\';
  Result := Trim(P);
end;


// find next " from StartAt (1-based), safe on out-of-range StartAt
function FindNextQuote(const S: string; StartAt: Integer): Integer;
var
  i, L: Integer;
begin
  L := Length(S);
  if (L = 0) or (StartAt < 1) or (StartAt > L) then
  begin
    Result := 0;
    Exit;
  end;

  for i := StartAt to L do
    if S[i] = '"' then
    begin
      Result := i;
      Exit;
    end;

  Result := 0;
end;

// Extract after ':' and strip quotes/commas  → for JSON-ish lines
function ExtractJsonValue(Line: string): string;
var
  i: Integer;
begin
  Result := '';
  i := Pos(':', Line);
  if i > 0 then
  begin
    Result := Trim(Copy(Line, i + 1, MaxInt));
    if (Length(Result) > 0) and (Result[1] = '"') then
    begin
      Delete(Result, 1, 1);
      i := Pos('"', Result);
      if i > 0 then
        Result := Copy(Result, 1, i - 1);
    end;
    // remove trailing comma if any
    if (Length(Result) > 0) and (Result[Length(Result)] = ',') then
      Delete(Result, Length(Result), 1);
  end;
end;


function ExtractVdfPathValue(const Line: string): string;
var
  p, q1, q2: Integer;
  val: string;
begin
  Result := '';
  p := Pos('"path"', LowerCase(Line));
  if p = 0 then Exit;

  // find first quote after "path"
  q1 := FindNextQuote(Line, p + 6);
  if q1 = 0 then Exit;
  q2 := FindNextQuote(Line, q1 + 1);
  if q2 = 0 then Exit;

  val := Copy(Line, q1 + 1, q2 - q1 - 1);
  Result := NormalizeLibraryPath(val);
end;


function CheckRoot(const steamappsRoot: string; var OutDir: string): Boolean;
var
  commonDir, gameDir: string;
begin
  Result := False;

  // 1) Look for Control folders directly
  commonDir := AddBackslash(steamappsRoot) + 'common';
  gameDir   := AddBackslash(commonDir) + 'Control';
  if DirExists(gameDir) then
  begin
    OutDir := gameDir;
    Result := True;
    Exit;
  end;

  gameDir := AddBackslash(commonDir) + 'Control Ultimate Edition';
  if DirExists(gameDir) then
  begin
    OutDir := gameDir;
    Result := True;
    Exit;
  end;

  // 2) If appmanifest exists, treat as a library and fall back to \common
  if FileExists(AddBackslash(steamappsRoot) + 'appmanifest_870780.acf') then
  begin
    if DirExists(AddBackslash(commonDir) + 'Control') then
      OutDir := AddBackslash(commonDir) + 'Control'
    else
      OutDir := commonDir;  // let user pick if subfolder varies
    Result := True;
    Exit;
  end;
end;

// formerly nested: now top-level
function ProbeSteamRoot(const steamappsRoot: string; var OutDir: string): Boolean;
begin
  Log('Steam: probing steamapps root ' + steamappsRoot);
  Result := DirExists(steamappsRoot) and CheckRoot(steamappsRoot, OutDir);
end;

function TryFindControlByHeuristic(var OutDir: string): Boolean;
var
  d: Integer;
  root: string;
begin
  Result := False;
  OutDir := '';

  // First, check Program Files installs (very common on Win 11)
  root := ExpandConstant('{pf32}') + '\Steam\steamapps';
  if ProbeSteamRoot(root, OutDir) then begin Result := True; Exit; end;

  root := ExpandConstant('{pf}') + '\Steam\steamapps';
  if ProbeSteamRoot(root, OutDir) then begin Result := True; Exit; end;

  // Then scan typical library roots on other drives
  for d := Ord('C') to Ord('Z') do
  begin
    root := Chr(d) + ':\Steam\steamapps';
    if ProbeSteamRoot(root, OutDir) then begin Result := True; Exit; end;

    root := Chr(d) + ':\SteamLibrary\steamapps';
    if ProbeSteamRoot(root, OutDir) then begin Result := True; Exit; end;

    // some users keep a Games\Steam structure
    root := Chr(d) + ':\Games\Steam\steamapps';
    if ProbeSteamRoot(root, OutDir) then begin Result := True; Exit; end;
  end;
end;

function FileExistsInSteam(): Boolean;
var
  SteamPath, VdfPath1, VdfPath2: string;
  Lines: TArrayOfString;
  i, j, n: Integer;
  LibRoots: array of string;
  Root, GameDir, GameExe: string;
  ExistsAlready: Boolean;
begin
  Result := False;
  SteamInstallPath := '';

  try
    if RegQueryStringValue(HKCU, 'Software\Valve\Steam', 'SteamPath', SteamPath) then
    begin
      // add default root
      SetArrayLength(LibRoots, 1);
      LibRoots[0] := SteamPath;

      // parse both possible locations of libraryfolders.vdf
      VdfPath1 := AddBackslash(SteamPath) + 'steamapps\libraryfolders.vdf';
      VdfPath2 := AddBackslash(SteamPath) + 'config\libraryfolders.vdf';

      if LoadStringsFromFile(VdfPath1, Lines) and (GetArrayLength(Lines) > 0) then
        for i := 0 to GetArrayLength(Lines) - 1 do
          try
              if Pos('"path"', Lines[i]) > 0 then
              begin
                Root := ExtractVdfPathValue(Lines[i]);   // now bullet-proof
                if Root <> '' then
                begin
                  ExistsAlready := False;
                  n := GetArrayLength(LibRoots);
                  for j := 0 to n - 1 do
                    if CompareText(LibRoots[j], Root) = 0 then begin ExistsAlready := True; Break; end;
                  if not ExistsAlready then
                  begin
                    SetArrayLength(LibRoots, n + 1);
                    LibRoots[n] := Root;
                    Log('Steam: library from VDF = ' + Root);
                  end;
                end;
                end;
            except
                Log('Steam exception while parsing line: ' + lines[i]);
          end;

      if LoadStringsFromFile(VdfPath2, Lines) and (GetArrayLength(Lines) > 0) then
        for i := 0 to GetArrayLength(Lines) - 1 do
          try
              if Pos('"path"', Lines[i]) > 0 then
              begin
                Root := ExtractVdfPathValue(Lines[i]);
                if Root <> '' then
                begin
                  ExistsAlready := False;
                  n := GetArrayLength(LibRoots);
                  for j := 0 to n - 1 do
                    if CompareText(LibRoots[j], Root) = 0 then begin ExistsAlready := True; Break; end;
                  if not ExistsAlready then
                  begin
                    SetArrayLength(LibRoots, n + 1);
                    LibRoots[n] := Root;
                    Log('Steam: library from VDF = ' + Root);
                  end;
                end;
                end;
            except
                Log('Steam exception while parsing line: ' + lines[i]);
          end;

      // probe each library
      for i := 0 to GetArrayLength(LibRoots) - 1 do
      begin
        Log('Steam: probing library "' + LibRoots[i] + '"');

        // try common folder names
        GameDir := AddBackslash(LibRoots[i]) + 'steamapps\common\Control';
        GameExe := GameDir + '\Control.exe';
        if FileExists(GameExe) or DirExists(GameDir) then
        begin
          SteamInstallPath := GameDir;
          Result := True;
          Log('Steam: found Control at ' + SteamInstallPath);
          Exit;
        end;

        GameDir := AddBackslash(LibRoots[i]) + 'steamapps\common\Control Ultimate Edition';
        GameExe := GameDir + '\Control.exe';
        if FileExists(GameExe) or DirExists(GameDir) then
        begin
          SteamInstallPath := GameDir;
          Result := True;
          Log('Steam: found Control at ' + SteamInstallPath);
          Exit;
        end;

        // fallback: appmanifest for Control (870780)
        if FileExists(AddBackslash(LibRoots[i]) + 'steamapps\appmanifest_870780.acf') then
        begin
          GameDir := AddBackslash(LibRoots[i]) + 'steamapps\common\Control';
          if DirExists(GameDir) then
            SteamInstallPath := GameDir
          else
            SteamInstallPath := AddBackslash(LibRoots[i]) + 'steamapps\common';
          Result := True;
          Log('Steam: found Control via appmanifest at ' + SteamInstallPath);
          Exit;
        end;
      end;
    end
    else
      Log('Steam: SteamPath not found in registry.');
  except
    Log('Steam: EXCEPTION while parsing VDF; falling back to drive scan.');
  end;

  // Final fallback: scan drives for Steam libraries (always runs if not found)
  if not Result then
  begin
    Log('Steam: starting drive-scan fallback.');
    if TryFindControlByHeuristic(SteamInstallPath) then
    begin
      Result := True;
      Log('Steam: heuristic found Control at ' + SteamInstallPath);
    end
    else
    begin
      Log('Steam: heuristic did not find Control.');
    end;
  end;
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
        if FileExistsInSteam() and (SteamInstallPath <> '') then
          SelectedInstallPath := SteamInstallPath
        else
          SelectedInstallPath := '';
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

var
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
  SteamPath: string;

begin
  if (SteamInstallPath <> '') then
    CheckAndAddPath(SteamInstallPath);

  if EpicInstallPath <> '' then
    CheckAndAddPath(EpicInstallPath);

  CheckAndAddPath(ExpandConstant('{app}'));
end;

function FileExistsInEpic(): Boolean;
var
  FindRec: TFindRec;
  ManifestDir, FilePath, InstallLoc: string;
  Content: AnsiString;
  startedFind: Boolean;
begin
  Result := False;
  EpicInstallPath := '';
  startedFind := False;

  try
    ManifestDir := ExpandConstant('{commonappdata}') +
                   '\Epic\EpicGamesLauncher\Data\Manifests';
    Log('Checking if the game is installed via Epic');

    if not DirExists(ManifestDir) then
      Exit;

    if FindFirst(ManifestDir + '\*.item', FindRec) then
    begin
      startedFind := True;
      repeat
        FilePath := ManifestDir + '\' + FindRec.Name;
        try
          if LoadStringFromFile(FilePath, Content) then
          begin
            // case-insensitive search for Control + InstallLocation
            if (Pos(UpperCase('"DisplayName": "Control"'), UpperCase(Content)) > 0) and
               (Pos('"InstallLocation":', Content) > 0) then
            begin
              // crude extraction: get the value after "InstallLocation":
              InstallLoc := Copy(Content, Pos('"InstallLocation":', Content) + 18, MaxInt);
              // strip up to the first quote
              if Pos('"', InstallLoc) > 0 then
              begin
                InstallLoc := Copy(InstallLoc, Pos('"', InstallLoc) + 1, MaxInt);
                if Pos('"', InstallLoc) > 0 then
                  InstallLoc := Copy(InstallLoc, 1, Pos('"', InstallLoc) - 1);
              end;
              EpicInstallPath := Trim(InstallLoc);
              Result := EpicInstallPath <> '';
              if Result then
              begin
                Log('Found Epic path: ' + EpicInstallPath);
                Exit;
              end;
            end;
          end;
        except
          Log('Warning: failed to read/parse "' + FilePath + '"; skipping.');
        end;
      until not FindNext(FindRec);
    end;
  except
    Log('Warning: exception in FileExistsInEpic(); treating as not installed.');
    Result := False;
  end;

  if startedFind then
  begin
    try
      FindClose(FindRec);
    except
      { ignore }
    end;
  end;
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


procedure InitializeWizard;
var
  InfoLabel1, InfoLabel2, InfoLabel3, InfoLabel4: TLabel;
  IsSteamInstalled, IsEpicInstalled: Boolean;
  CurrentTop: Integer;
begin
  Log('IW: start');
  CreateDisclaimerPage();
  Log('IW: disclaimer page created');

  MyPage := CreateCustomPage(
    wpSelectDir,
    'Choose Game Versions',
    'Select which game versions to install the mod for.'
  );
  Log('IW: custom page created');

  // --- Detection (Steam for all; Epic only on Win11+) ---

  Log('IW: steam detection done');
  try
    IsSteamInstalled := FileExistsInSteam();
  except
    Log('IW: EXCEPTION in FileExistsInSteam; treating as not installed (using manual).');
    IsSteamInstalled := False;
  end;
  //if IsWindows11OrNewer then
  //begin
  //  Log('IW: win11+, attempting epic detect');
    try
      IsEpicInstalled := FileExistsInEpic();
    except
      Log('IW: epic detect raised, treating as not installed');
      IsEpicInstalled := False;
    end;
    Log('IW: epic detection done (win11+)');
//  end
//  else
//  begin
//    // Win10: do NOT probe Epic; hide the option
//    IsEpicInstalled := False;
//    EpicInstallPath := '';
//    Log('IW: win10, epic hidden');
//  end;  // <-- CLOSE the else BEFORE building UI

  // --- Build labels (explicitly create them) ---
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

  // --- Create Manual controls first (robust on Win10) ---
  try
    Log('IW: creating Manual checkbox');
    ManualCheckbox := TCheckBox.Create(WizardForm);
    ManualCheckbox.Parent := MyPage.Surface;
    ManualCheckbox.Top := CurrentTop;
    ManualCheckbox.Left := ScaleX(0);
    ManualCheckbox.Width := ScaleX(300);
    ManualCheckbox.Height := ScaleY(20);
    ManualCheckbox.Caption := 'Install to custom path:';
    ManualCheckbox.OnClick := @ManualCheckboxClick;
    ManualCheckbox.Checked := False;  // will uncheck if Steam shows up
    CurrentTop := CurrentTop + ScaleY(24);

    Log('IW: creating Manual path edit + Browse');
    ManualPathEdit := TEdit.Create(WizardForm);
    ManualPathEdit.Parent := MyPage.Surface;
    ManualPathEdit.Top := CurrentTop;
    ManualPathEdit.Left := ScaleX(0);
    ManualPathEdit.Width := ScaleX(300);
    ManualPathEdit.Height := ScaleY(25);
    ManualPathEdit.Text := 'C:\Games\Control';

    ManualBrowseButton := TButton.Create(WizardForm);
    ManualBrowseButton.Parent := MyPage.Surface;
    ManualBrowseButton.Top := CurrentTop;
    ManualBrowseButton.Left := ManualPathEdit.Left + ManualPathEdit.Width + ScaleX(8);
    ManualBrowseButton.Width := ScaleX(75);
    ManualBrowseButton.Height := ScaleY(25);
    ManualBrowseButton.Caption := 'Browse...';
    ManualBrowseButton.OnClick := @BrowseManualPath;

    Log('IW: manual controls created');
  except
    Log('IW: ERROR creating manual controls; minimalizing.');
    if ManualCheckbox = nil then
    begin
      ManualCheckbox := TCheckBox.Create(WizardForm);
      ManualCheckbox.Parent := MyPage.Surface;
      ManualCheckbox.Top := CurrentTop;
      ManualCheckbox.Left := ScaleX(0);
      ManualCheckbox.Caption := 'Install to custom path:';
      ManualCheckbox.Checked := True;
    end;
  end;

  // --- Steam (guarded so a failure doesn't kill the page) ---
  if IsSteamInstalled then
  begin
    try
      Log('IW: creating Steam checkbox (path=' + SteamInstallPath + ')');
      SteamCheckbox := TCheckBox.Create(WizardForm);
      SteamCheckbox.Parent := MyPage.Surface;
      SteamCheckbox.Top := InfoLabel4.Top + ScaleY(24);
      if Assigned(ManualBrowseButton) then
        CurrentTop := ManualBrowseButton.Top + ManualBrowseButton.Height + ScaleY(8)
      else if Assigned(ManualCheckbox) then
        CurrentTop := ManualCheckbox.Top + ScaleY(28)
      else
        CurrentTop := InfoLabel4.Top + ScaleY(24);
      SteamCheckbox.Top := CurrentTop;
      SteamCheckbox.Left := ScaleX(0);
      SteamCheckbox.Width := ScaleX(300);
      SteamCheckbox.Height := ScaleY(20);
      SteamCheckbox.Caption := 'Install for Steam';
      SteamCheckbox.Checked := True;

      if Assigned(ManualCheckbox) then
        ManualCheckbox.Checked := False;

      Log('IW: steam checkbox created');
    except
      Log('IW: WARNING Steam checkbox creation failed; continuing without Steam option.');
    end;
  end;

  // --- Epic (create checkbox if EpicInstallPath was found) ---
  if IsEpicInstalled and (EpicInstallPath <> '') then
  begin
    try
      // stack Epic below whatever was created last
      if Assigned(SteamCheckbox) then
        CurrentTop := SteamCheckbox.Top + SteamCheckbox.Height + ScaleY(8)
      else if Assigned(ManualBrowseButton) then
        CurrentTop := ManualBrowseButton.Top + ManualBrowseButton.Height + ScaleY(8)
      else if Assigned(ManualCheckbox) then
        CurrentTop := ManualCheckbox.Top + ScaleY(28)
      else
        CurrentTop := InfoLabel4.Top + ScaleY(24);

      Log('IW: creating Epic checkbox (path=' + EpicInstallPath + ')');
      EpicCheckbox := TCheckBox.Create(WizardForm);
      EpicCheckbox.Parent := MyPage.Surface;
      EpicCheckbox.Top := CurrentTop;
      EpicCheckbox.Left := ScaleX(0);
      EpicCheckbox.Width := ScaleX(300);
      EpicCheckbox.Height := ScaleY(20);
      EpicCheckbox.Caption := 'Install for Epic';
      // Default: if Steam is present, leave Epic unchecked; otherwise pre-check Epic
      EpicCheckbox.Checked := not IsSteamInstalled;

      // If Epic is preselected, uncheck Manual so only one is on by default
      if EpicCheckbox.Checked and Assigned(ManualCheckbox) then
        ManualCheckbox.Checked := False;

      // advance for anything else that might be added later
      CurrentTop := EpicCheckbox.Top + EpicCheckbox.Height + ScaleY(8);

      Log('IW: epic checkbox created');
    except
      Log('IW: WARNING Epic checkbox creation failed; continuing without Epic option.');
    end;
  end;

  if not IsSteamInstalled and not IsEpicInstalled then
  begin
    Log('IW: steam NOT installed/detected; leaving Manual default');
    if Assigned(ManualCheckbox) then
      ManualCheckbox.Checked := True;
  end;


  ManualCheckboxClick(nil);
  Log('IW: checkboxes wired');
end;

