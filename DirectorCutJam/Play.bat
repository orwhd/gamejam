@echo off
setlocal
set "PROJECT_DIR=%~dp0"
set "PACKAGED_EXE=%PROJECT_DIR%..\PlayableBuild\Windows\DirectorCutJam.exe"
if exist "%PACKAGED_EXE%" (
    "%PACKAGED_EXE%" /Game/Maps/Firefighter -ResX=1600 -ResY=900 -windowed %*
    exit /b %ERRORLEVEL%
)
if "%UE_EDITOR%"=="" set "UE_EDITOR=D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
if not exist "%UE_EDITOR%" (
    echo UE 5.8 editor not found at "%UE_EDITOR%" and standalone build missing at "%PACKAGED_EXE%". 1>&2
    exit /b 1
)
"%UE_EDITOR%" "%PROJECT_DIR%DirectorCutJam.uproject" /Game/Maps/Firefighter -game -ResX=1600 -ResY=900 -windowed %*
