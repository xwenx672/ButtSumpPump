@echo off
set "batdir=%~dp0"
pushd "%batdir%"

net session > NUL 2>&1
if %errorlevel% == 0 (
break
) else (
nircmd.exe elevate "%batdir%\%~n0.bat"
exit
)


setlocal ENABLEDELAYEDEXPANSION
:loop:
rmdir /s /q "%userprofile%\.ssh" > NUL
netsh interface ip delete arpcache

timeout 1 /nobreak > NUL
cls

set "ip="
for /f "tokens=12 delims=: " %%A in ('
    ipconfig ^| findstr /R /C:"Default Gateway"
') do (
    set "ip=%%A"
)

if not defined ip (
echo Could not determine local IPv4 address.
timeout 5 /nobreak > NUL

goto loop
)


rem Trim leading space(s)
set "ip=%ip: =%"

rem Extract subnet A.B.C from A.B.C.D
for /f "tokens=1-4 delims=." %%A in ("%ip%") do (
    set "subnet=%%A.%%B.%%C"
)


rem echo Local IP   : %ip%
echo Subnet     : %subnet%.0/24
echo Scanning hosts to populate ARP table...
echo (This may take a little while.)
echo.

rem === Ping sweep to populate ARP cache ===
for /L %%I in (1,1,254) do (
    start /B "" ping -n 1 -w 5 %subnet%.%%I > NUL
)
arp -a
timeout 60
goto loop