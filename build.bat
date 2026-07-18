@echo off
setlocal

if exist microchat.exe (
    echo Removing old microchat.exe...
    del microchat.exe
)

echo Building dialog...
windres --target=pe-i386 servconn.rc -o servconn.o
if %errorlevel% neq 0 (
    echo [ERROR] Dialog FAILED
    exit /b 1
)
echo [OK] Dialog built

echo Compiling MicroChat...
gcc main.c network.c ui.c utils.c servconn.o -o microchat.exe -m32 -lgdi32 -lws2_32 -lcomctl32 -mwindows -s -Wl,--gc-sections -Wall -Wextra -municode
if %errorlevel% neq 0 (
    echo [ERROR] Build FAILED
    exit /b 1
)
echo [OK] Build successful: microchat.exe

endlocal