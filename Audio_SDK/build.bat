@echo off
setlocal enabledelayedexpansion

rem 在这里修改andes安装的根目录
for %%I in ("%~dp0..\AndesToolsLite") do set "AndesRoot=%%~fI"
rem 验证路径是否存在
if not exist "%AndesRoot%" (
    echo 错误: 找不到 AndesToolsLite 目录: %AndesRoot%
    echo 请检查目录结构是否正确
    pause
    exit /b 1
)

set "AndesRootUtils=%AndesRoot%\utils"
set "AndesRootIce=%AndesRoot%\ice"
set "AndesRootCygwin=%AndesRoot%\cygwin\bin"
set "AndesRootCygwin_lib=%AndesRoot%\cygwin\bin\lib"
set "AndesRootGcc=%AndesRoot%\toolchains\nds32le-elf-mculib-v3s\bin"

set PATH=%AndesRoot%;%AndesRootUtils%;%AndesRootCygwin%;%AndesRootCygwin_lib%;%AndesRootGcc%;%PATH%

set "HomeRoot=%cd%"

echo 开始编译...
echo.

echo 清理旧的编译输出...
if exist debug rd /s /q debug

nds_ldsag.exe -t tools\nds32_template.txt tools\BP15x.sag -o tools\nds32-ae210p.ld
echo 生成Makefile...
tools\makefile_gen.exe -i makefile.ini -d .\
cd debug
make all

rem 返回原始目录
cd "%HomeRoot%"

echo 生成MVA...
tools\merge_script\merge.exe
tools\merge_script\Andes_MVAGenerate.exe

echo.
echo 编译完成!
