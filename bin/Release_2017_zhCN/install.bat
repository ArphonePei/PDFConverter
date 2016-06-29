@echo off
SET PLATFORM=Win32
if "%PROCESSOR_ARCHITECTURE%"=="x86" set "SYSTEM=Win32"
if "%PROCESSOR_ARCHITECTURE%"=="AMD64" set "SYSTEM=x64"
SET "SELFDIR=%~dp0"

title 安装PDFConverter for ZWCAD 2017 简体中文版
color 2f

echo *************************************************************
echo *      本项目遵循 GNU-GPL v3 开源协议                       *
echo *      任何人可以从以下地址获取本程序源代码：               *
echo *      https://github.com/ArphonePei/PDFConverter           *
echo *************************************************************
echo.
echo.
echo *************************************************************
echo   请选择要安装PdfConverter的CAD版本:
:: 查找安装的版本
echo         中望CAD 2017 简体中文版............^<1^>
echo         中望CAD机械版 2017 简体中文........^<2^>
echo         中望CAD建筑版 2017 简体中文........^<3^>
echo         退出...............................^<0^>
echo         安装所有版本.......................^<回车^>
echo.
echo         访问中望CAD........................^<8^>
echo         访问本项目.........................^<9^>
echo *************************************************************
echo.
echo.

:INPUT
set "VER="
set /p VER=请输入要选择安装到的版本:
if "%VER%"=="0" EXIT

if "%VER%"=="1" (
 call :InstallZwcad
 if ERRORLEVEL 1 goto NOTFOUND
 goto SUCCESS
)

if "%VER%"=="2" (
 call :InstallZwcadm
 if ERRORLEVEL 1 goto NOTFOUND
 goto SUCCESS
)

if "%VER%"=="3" (
 call :InstallZwcada
 if ERRORLEVEL 1 goto NOTFOUND
 goto SUCCESS
)

if "%VER%"=="8" (
START http://www.zwcad.com
goto INPUT
)

if "%VER%"=="9" (
START https://github.com/ArphonePei/PDFConverter
goto INPUT
)

if "%VER%"=="" (
 call :InstallZwcad
 call :InstallZwcadm
 call :InstallZwcada
 pause
 exit
)

echo 无效输入，请重新输入
goto INPUT

:InstallZwcad
echo 正在安装到中望CAD 2017
set ZWCAD=ZWCAD
set "PRODUCT=中望CAD 2017"

call :Install
exit /b %errorlevel%

:InstallZwcadm
echo 正在安装到中望CAD机械版 2017
set ZWCAD=ZWCADM
set "PRODUCT=中望CAD机械版 2017"

call :Install
exit /b %errorlevel%


:InstallZwcada
echo 正在安装到中望CAD建筑版 2017
set ZWCAD=ZWCADA
set "PRODUCT=中望CAD建筑版 2017"

call :Install
exit /b %errorlevel%

:Install
if "%PLATFORM%"=="Win32" if "%SYSTEM%"=="x64" set "REGSTR=SOFTWARE\WOW6432Node\ZWSOFT\%ZWCAD%\2017\zh-CN"
if "%REGSTR%"=="" set "REGSTR=SOFTWARE\ZWSOFT\%ZWCAD%\2017\zh-CN"

for /f "tokens=2*" %%a in ('reg query "HKLM\%REGSTR%" /v Location 2^>nul') do set "LOCATION=%%b"
if "%LOCATION%"=="" exit /b 1

if not exist "%LOCATION%ZWCAD.EXE" exit /b 1

echo Windows Registry Editor Version 5.00 >inst.reg 
echo. >> inst.reg
echo [HKEY_CURRENT_USER\%REGSTR%\Applications\PdfConverter] >>inst.reg
echo "LOADER"="%SELFDIR:\=\\%PdfConverter.zrx" >>inst.reg
echo "LOADCTRLS"=dword:04 >>inst.reg
echo [HKEY_CURRENT_USER\%REGSTR%\Applications\PdfConverter\Groups] >>inst.reg
echo "ZOSP_PDF_TOOLs"="ZOSP_PDF_TOOLs" >>inst.reg
echo [HKEY_CURRENT_USER\%REGSTR%\Applications\PdfConverter\Commands] >>inst.reg
echo "PDF2DWG"="PDF2DWG" >>inst.reg
echo "PDF2DXF"="PDF2DXF" >>inst.reg

regedit /s inst.reg
::del /q inst.reg

exit /b 0



:NOTFOUND
echo 没有在您的电脑上找到 %PRODUCT%，请先下载安装或安装到其它版本。
echo 从中望CAD官网获取 %PRODUCT% ： http://www.zwcad.com
echo.
goto INPUT


:SUCCESS
echo 安装到 %PRODUCT% 成功
goto INPUT



