@echo off
@rem Batch file for Andes Technology Product to launch Cygwin
set PATH=%~dp0bin
set CYGPATH=cygpath
IF EXIST "%PATH%\bash.exe" set SHELL=/bin/bash
"%PATH%\bash.exe" --login -i

:END
