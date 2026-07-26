@echo off
setlocal

set SYM=0
set CLEAN=0
set HELP=0

:parse_args
if "%1"=="" goto :done_parse
if /i "%1"=="/sym" set SYM=1
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
    echo build.bat [/help] [/clean] [/sym]
    echo.
    echo /help  - Show this help.
    echo /clean - Delete existing compiled files.
    echo /sym   - Embed debug symbols and do not delete unused code.
    goto end
)

echo ==== Build parameters =====
echo Clean: %CLEAN%
echo Sym: %SYM%
echo ===========================
echo.

if %CLEAN%==1 (
    echo Deleting previous build files...
    del microchat.exe
    del servconn.o
    echo Done.
    goto end
)
if exist microchat.exe (
    echo Warning! One or several previous build files has been detected. Please run "build /clean" to remove them.
)

echo Building dialog...
windres --target=pe-i386 servconn.rc -o servconn.o
if %errorlevel% neq 0 (
    echo [ERROR] Dialog FAILED
    exit /b 1
)
echo [OK] Dialog built

echo Compiling MicroChat...
if %SYM%==1 (
    echo + embedding Debug symbols
    gcc main.c network.c ui.c utils.c servconn.o -o microchat.exe -m32 -lgdi32 -lws2_32 -lcomctl32 -mwindows -Wall -Wextra -municode
    if %errorlevel% neq 0 (
        echo [ERROR] Build FAILED
        exit /b 1
    )
    goto end
)

gcc main.c network.c ui.c utils.c servconn.o -o microchat.exe -m32 -lgdi32 -lws2_32 -lcomctl32 -mwindows -s -Wl,--gc-sections -Wall -Wextra -municode
if %errorlevel% neq 0 (
    echo [ERROR] Build FAILED
    exit /b 1
)
echo [OK] Build successful: microchat.exe

endlocal

:end