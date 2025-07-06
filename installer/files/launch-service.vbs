Set WshShell = CreateObject("WScript.Shell")

If WScript.Arguments.Count = 0 Then
    WScript.Echo "No argument passed."
    WScript.Quit 1
End If

path = WScript.Arguments(0)

' Extract directory and exe file name
Set fso = CreateObject("Scripting.FileSystemObject")
folder = fso.GetParentFolderName(path)
exe = fso.GetFileName(path)

' Properly quoted command
command = "cmd.exe /q /c start """ & """" & " /B /MIN /D """ & folder & """ """ & exe & """"

'WScript.Echo command


' Optional debug
' WScript.Echo command

WshShell.Run command, 0, False

