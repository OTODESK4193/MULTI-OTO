@echo off
set EXE_PATH=%~dp0out\build\MultiOto_artefacts\Release\Standalone\MULTI-OTO.exe

if exist "%EXE_PATH%" (
    echo Launching MULTI-OTO Standalone...
    start "" "%EXE_PATH%"
) else (
    echo Error: MULTI-OTO.exe not found. Please build the project first.
    pause
)
