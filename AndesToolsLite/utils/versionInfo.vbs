set args = WScript.Arguments
set objWSHShell = CreateObject("WScript.Shell")
Set fso = CreateObject("Scripting.FileSystemObject")
set oEnv = objWSHShell.Environment("Process")

systemDirectory = fso.GetSpecialFolder(1)
oEnv("SystemDirectory") = systemDirectory

dllname = objWSHShell.ExpandEnvironmentStrings(args(0))
if fso.FileExists(dllname) = false then
	dllname = fso.BuildPath(systemDirectory, dllname)
end if
WScript.Echo fso.GetFileVersion(dllname)
Wscript.Quit