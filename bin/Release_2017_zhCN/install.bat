@echo off
SET PLATFORM=Win32
if "%PROCESSOR_ARCHITECTURE%"=="x86" set "SYSTEM=Win32"
if "%PROCESSOR_ARCHITECTURE%"=="AMD64" set "SYSTEM=x64"
SET "SELFDIR=%~dp0"

title ��װPDFConverter for ZWCAD 2017 �������İ�
color 2f

echo *************************************************************
echo *      ����Ŀ��ѭ GNU-GPL v3 ��ԴЭ��                       *
echo *      �κ��˿��Դ����µ�ַ��ȡ������Դ���룺               *
echo *      https://github.com/ArphonePei/PDFConverter           *
echo *************************************************************
echo.
echo.
echo *************************************************************
echo   ��ѡ��Ҫ��װPdfConverter��CAD�汾:
:: ���Ұ�װ�İ汾
echo         ����CAD 2017 �������İ�............^<1^>
echo         ����CAD��е�� 2017 ��������........^<2^>
echo         ����CAD������ 2017 ��������........^<3^>
echo         �˳�...............................^<0^>
echo         ��װ���а汾.......................^<�س�^>
echo.
echo         ��������CAD........................^<8^>
echo         ���ʱ���Ŀ.........................^<9^>
echo *************************************************************
echo.
echo.

:INPUT
set "VER="
set /p VER=������Ҫѡ��װ���İ汾:
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

echo ��Ч���룬����������
goto INPUT

:InstallZwcad
echo ���ڰ�װ������CAD 2017
set ZWCAD=ZWCAD
set "PRODUCT=����CAD 2017"

call :Install
exit /b %errorlevel%

:InstallZwcadm
echo ���ڰ�װ������CAD��е�� 2017
set ZWCAD=ZWCADM
set "PRODUCT=����CAD��е�� 2017"

call :Install
exit /b %errorlevel%


:InstallZwcada
echo ���ڰ�װ������CAD������ 2017
set ZWCAD=ZWCADA
set "PRODUCT=����CAD������ 2017"

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
echo û�������ĵ������ҵ� %PRODUCT%���������ذ�װ��װ�������汾��
echo ������CAD������ȡ %PRODUCT% �� http://www.zwcad.com
echo.
goto INPUT


:SUCCESS
echo ��װ�� %PRODUCT% �ɹ�
goto INPUT



