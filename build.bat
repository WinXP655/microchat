@echo off
setlocal

set DBG=0
set CLEAN=0
set HELP=0
set PREVBLD=0

:parse_args
if "%1"=="" goto :done_parse
if /i "%1"=="/dbg" set DBG=1
if /i "%1"=="/clean" set CLEAN=1
if /i "%1"=="/help" set HELP=1
shift
goto :parse_args
:done_parse

if %HELP%==1 (
    echo MicroChat Build Environment
    echo This script is used to build MicroChat.
    echo.
    echo Supported arguments:
    echo build.bat [/help] [/clean] [/dbg]
    echo.
    echo /help  - Show this help.
    echo /clean - Delete existing compiled files.
    echo /dbg   - Embed debug symbols, debug menu and do not optimize code.
    goto end
)

echo Build parameters: Clean: %CLEAN% Debug: %DBG%
echo.

if %CLEAN%==1 (
    echo Deleting previous build files...
    if exist microchat.exe del microchat.exe /f
    if exist microchat_dbg.exe del microchat_dbg.exe /f
    if exist servconn.o del servconn.o /f
    echo Done.
    goto end
)

for %%F in (microchat.exe microchat_dbg.exe servconn.o) do (
    if exist "%%F" set PREVBLD=1
)

if %PREVBLD%==1 echo Warning! One or several previous build files has been detected. Please run "build /clean" to remove them.

echo Building dialog...
windres -c 65001 --target=pe-i386 servconn.rc -o servconn.o
if %errorlevel% neq 0 (
    echo [ERROR] Dialog FAILED
    exit /b 1
)
echo [OK] Dialog built

echo Compiling MicroChat...
if %DBG%==1 (
    echo Build type: Debug.
    gcc main.c network.c ui.c utils.c servconn.o debug.c -o microchat_dbg.exe -m32 -lgdi32 -lws2_32 -lcomctl32 -mwindows -Wall -Wextra -municode -D_DEBUG -Wl,--gc-sections 
    if %errorlevel% neq 0 (
        echo [ERROR] Build FAILED
        exit /b 1
    )
    echo [OK] Build successful: microchat_dbg.exe ^(debug^)
    goto end
)

gcc main.c network.c ui.c utils.c servconn.o debug.c -o microchat.exe -m32 -lgdi32 -lws2_32 -lcomctl32 -mwindows -Os -s -ffunction-sections -fdata-sections -Wl,--gc-sections -Wall -Wextra -municode -Wl,--gc-sections -Wl,--subsystem,windows:5.0
if %errorlevel% neq 0 (
    echo [ERROR] Build FAILED
    exit /b 1
)
echo [OK] Build successful: microchat.exe

endlocal

:end