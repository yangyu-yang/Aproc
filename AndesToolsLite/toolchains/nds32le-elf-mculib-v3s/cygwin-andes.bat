@echo off
@rem Batch file for Andes Technology Product to launch Cygwin
Set HOME=%~dp0
For %%A in ("%HOME:~0,-1%") do (
    Set CYGPATH=%%~dpA
)
For %%A in ("%CYGPATH:~0,-1%") do (
    Set CYGPATH=%%~dpA
)
Set HOME=%HOME%bin
set PATH=%CYGPATH%cygwin\bin;%CYGPATH%utils;%HOME%;%PATH%
IF EXIST "%CYGPATH%COPILOT\bin\copilot.exe" set PATH=%CYGPATH%COPILOT\bin;%PATH%
IF EXIST "%CYGPATH%cygwin\bin\bash.exe" set SHELL=/bin/bash
set CYGPATH=cygpath
"..\..\cygwin\bin\mintty.exe" -

:END

