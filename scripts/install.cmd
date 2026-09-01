@echo off
:: ============================================
:: Assetto Corsa EVO Head Tracking - Install
:: ============================================
:: Thin wrapper - install body lives in cameraunlock-core/scripts/install-body-asi.cmd,
:: staged into the release ZIP's shared/ by Copy-SharedBundle. To change
:: install behaviour edit the body, not this wrapper. Everything below the
:: CONFIG BLOCK is copied verbatim from
:: cameraunlock-core/scripts/templates/install-wrapper-asi.cmd.
:: ============================================

:: --- CONFIG BLOCK ---
set "GAME_ID=assetto-corsa-evo"
set "MOD_DISPLAY_NAME=Assetto Corsa EVO Head Tracking"
set "MOD_DLLS=AssettoCorsaEvoHeadTracking.asi"
set "MOD_INTERNAL_NAME=AssettoCorsaEvoHeadTracking"
set "MOD_VERSION=1.1.2"
set "STATE_FILE=.headtracking-state.json"
set "FRAMEWORK_TYPE=ASILoader"
set "ASI_LOADER_NAME=dinput8.dll"
set "MOD_CONTROLS=End / Ctrl+Shift+Y toggle tracking, PgUp / Ctrl+Shift+G cycle tracking mode"
:: --- END CONFIG BLOCK ---

:: Pin delayed expansion off before `%*` is expanded on the `call` below.
:: Under `cmd /V:ON`, or with DelayedExpansion=1 in
:: HKCU\Software\Microsoft\Command Processor, cmd.exe eats a `!` out of the
:: expanded line, and a real game path like C:\Games\Oh! My Game reaches the
:: body already mangled. The body pins expansion off at its own outer scope
:: too, but that is one `call` too late to save the argument it was handed.
setlocal disabledelayedexpansion

set "WRAPPER_DIR=%~dp0"
set "_BODY=%WRAPPER_DIR%shared\install-body-asi.cmd"
if not exist "%_BODY%" set "_BODY=%WRAPPER_DIR%..\cameraunlock-core\scripts\install-body-asi.cmd"
if not exist "%_BODY%" (
    echo ERROR: install-body-asi.cmd not found in shared\ or ..\cameraunlock-core\scripts\.
    echo If this is a release ZIP, re-download it from GitHub ^(corrupt installer^).
    echo If this is the dev tree, run: git submodule update --init --recursive
    exit /b 1
)
call "%_BODY%" %*
exit /b %errorlevel%