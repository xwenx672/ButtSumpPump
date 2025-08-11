@echo off
pushd "%~dp0"
set "batdir=%~dp0"
:start:
cls
for /f "tokens=1,2*" %%I in ('dir "%batdir%build\esp32.esp32.esp32\%latestFile%" /t:w ^| findstr /i "%latestFile%"') do (
    set "currentMod=%%I %%J"
)

:loop:
cls
rem Get the newest file's last modified timestamp
for /f "delims=" %%A in ('dir "%batdir%build\esp32.esp32.esp32\*.ino.bin" /b /a-d /o-d 2^>nul') do (
    set "latestFile=%%A"
    goto gotFile
)
echo Does not exist
timeout 1 /nobreak > NUL
goto loop

:gotFile:
echo Found ino
for /f "tokens=1,2*" %%I in ('dir "%batdir%build\esp32.esp32.esp32\%latestFile%" /t:w ^| findstr /i "%latestFile%"') do (
    set "currentMod=%%I %%J"
)
timeout 1 /nobreak > NUL

rem Compare with last recorded timestamp
if "%lastMod%"=="%currentMod%" (
    echo Same file, wait a bit and loop
    timeout /nobreak 1 > NUL
    goto loop
)

rem New or updated file detected
set "lastMod=%currentMod%"
echo New file or updated: %latestFile% at %currentMod%

:: Disable Windows Defender real-time protection
rem powershell -Command "Set-MpPreference -DisableRealtimeMonitoring $true"

set IP=192.168.1.184
set "ESPOTA=%localappdata%\Arduino15\packages\esp32\hardware\esp32\3.2.0\tools\espota.py"
set "AUTH=Blueeyeswhitedragon10!"

:: Find the most recent .ino.bin file in build folder
for /f "delims=" %%i in ('dir /b /od /a:-d "build\esp32.esp32.esp32\*.ino.bin"') do set "BIN=build\esp32.esp32.esp32\%%i"

if not exist "%BIN%" (
    echo ERROR: No .ino.bin file found in build folder.
    pause
    exit /b 1
)

echo Using firmware: %BIN%
echo.
:retry
echo Testing connection to %IP%
ping -n 1 -w 2000 %IP% > NUL
if %errorlevel% == 1 (
echo No connection
timeout 1 /nobreak > NUL
cls
goto retry
) else (
echo Connection to %IP% ok
)
echo Trying OTA upload...
python "%ESPOTA%" -i %IP% -p 3232 --auth="%AUTH%" --progress -f "%BIN%"
if errorlevel 1 (
    echo Upload failed. Retrying in 3 seconds...
    timeout /t 3 /nobreak > nul
    cls
    goto retry
) else (
    echo Upload successful!
)

:: Re-enable Windows Defender real-time protection
rem powershell -Command "Set-MpPreference -DisableRealtimeMonitoring $false"

timeout 5 /nobreak
