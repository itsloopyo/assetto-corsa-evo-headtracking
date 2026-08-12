@echo off
:: ============================================
:: CameraUnlock Uninstall Template (Unified)
:: ============================================
:: Source of truth: cameraunlock-core/scripts/templates/uninstall.cmd.
:: Copy to <mod>/scripts/uninstall.cmd, edit CONFIG BLOCK, leave the rest
:: alone. Contract: see ~/.claude/CLAUDE.md "install.cmd / uninstall.cmd
:: - Unified Launcher Contract".
::
:: One template, all loader variants. Dispatch is by FRAMEWORK_TYPE which
:: MUST match what install.cmd wrote to the state file. Supported values:
::
::   BepInEx      - removes <game>/BepInEx/, winhttp.dll, doorstop files
::   MelonLoader  - removes <game>/MelonLoader/, version.dll, dobby.dll
::   MonoCecil    - restores Assembly-CSharp.dll from .original backup
::   ASILoader    - removes <exe-dir>/winmm.dll (or dinput8.dll)
::   REFramework  - removes <game>/dinput8.dll and <game>/reframework/
::   None         - shim-only; restores shim DLLs from .backup if present
::
:: Launcher CLI: uninstall.cmd [GAME_PATH] [/y] [/force]
::   /y      - non-interactive; skip every pause and prompt
::   /force  - remove loader even if state says installed_by_us=false
:: ============================================

:: --- CONFIG BLOCK ---
set "GAME_ID=assetto-corsa-evo"
set "MOD_DISPLAY_NAME=Assetto Corsa EVO Head Tracking"
set "MOD_DLLS=AssettoCorsaEvoHeadTracking.asi"
set "MOD_INTERNAL_NAME=AssettoCorsaEvoHeadTracking"
set "STATE_FILE=.headtracking-state.json"
set "FRAMEWORK_TYPE=ASILoader"
set "LEGACY_DLLS="

:: --- Loader-specific config (leave the ones that don't apply blank) ---
:: MonoCecil: used to find + restore the original Assembly-CSharp.dll.
set "MANAGED_SUBFOLDER="
set "ASSEMBLY_DLL="
:: MonoCecil REQUIRED: the patcher's marker (must match install.cmd /
:: BootstrapPatcher.cs). The uninstall body refuses to restore a patched
:: .original over the game assembly. Leave blank for non-MonoCecil mods.
set "PATCH_MARKER="
:: MonoCecil: extra files to also remove from MANAGED_SUBFOLDER (config/log
:: files left behind by the mod itself).
set "MANAGED_EXTRAS="
:: ASILoader: filename the ASI DLL was renamed to. Defaults to winmm.dll.
set "ASI_LOADER_NAME=dinput8.dll"
:: --- END CONFIG BLOCK ---

call :detect_yes_flag %*
call :main %*
set "_EC=%errorlevel%"
if not defined YES_FLAG ( echo. & pause )
exit /b %_EC%

:: ============================================
:: Pre-scan args at outer scope so YES_FLAG propagates to the post-:main
:: pause check. :main's arg parser sets its own (local) YES_FLAG too, but
:: cmd.exe discards local vars when setlocal pops on `exit /b`, so without
:: this pre-scan the post-:main `if not defined YES_FLAG` always pauses
:: and /y can't make the script headless. Quoted-string form is required
:: here - bracket form `if [%~1]==[/y]` does NOT quote, so a path arg
:: containing whitespace ("C:\...\Gone Home") splits across the brackets
:: and crashes cmd with "[Home]==[/y] was unexpected at this time". The
:: trailing-backslash hazard the bracket form was working around is moot
:: with `%~1`: it strips the launcher's surrounding quotes before the
:: comparison, so a value like `C:\foo\` can't escape the closing `"`.
:: ============================================
:detect_yes_flag
if "%~1"=="" exit /b 0
if /i "%~1"=="/y"    set "YES_FLAG=1"
if /i "%~1"=="-y"    set "YES_FLAG=1"
if /i "%~1"=="--yes" set "YES_FLAG=1"
shift
goto :detect_yes_flag

:main
setlocal enabledelayedexpansion

:: Capture script dir BEFORE the arg parser runs. Inside `call :main`,
:: `shift` rotates %0 too, so %~dp0 read after shifts resolves to the
:: dirname of the first arg (e.g. C:\ for /y) instead of the script.
set "SCRIPT_DIR=%~dp0"

:: -------- Arg parser (canonical, do not modify) --------
set "YES_FLAG="
set "FORCE_FLAG="
set "_GIVEN_PATH="
:parse_args
if "%~1"=="" goto :args_done
set "_ARG=%~1"
if /i "!_ARG!"=="/y"      ( set "YES_FLAG=1"   & shift & goto :parse_args )
if /i "!_ARG!"=="-y"      ( set "YES_FLAG=1"   & shift & goto :parse_args )
if /i "!_ARG!"=="--yes"   ( set "YES_FLAG=1"   & shift & goto :parse_args )
if /i "!_ARG!"=="/force"  ( set "FORCE_FLAG=1" & shift & goto :parse_args )
if /i "!_ARG!"=="--force" ( set "FORCE_FLAG=1" & shift & goto :parse_args )
if "!_ARG:~0,2!"=="--" ( echo ERROR: unknown flag "!_ARG!" & exit /b 2 )
if "!_ARG:~0,1!"=="/"  ( echo ERROR: unknown flag "!_ARG!" & exit /b 2 )
if "!_ARG:~0,1!"=="-"  ( echo ERROR: unknown flag "!_ARG!" & exit /b 2 )
if not defined _GIVEN_PATH (
    if exist "!_ARG!\" ( set "_GIVEN_PATH=!_ARG!" & shift & goto :parse_args )
)
echo ERROR: unrecognised argument "!_ARG!"
exit /b 2
:args_done

echo.
echo === %MOD_DISPLAY_NAME% - Uninstall ===
echo.

:: -------- Resolve game path via shared shim --------
set "_SHIM=%SCRIPT_DIR%shared\find-game.ps1"
if not exist "%_SHIM%" set "_SHIM=%SCRIPT_DIR%..\cameraunlock-core\scripts\find-game.ps1"
if not exist "%_SHIM%" (
    echo ERROR: find-game.ps1 not found in shared\ or ..\cameraunlock-core\scripts\.
    echo If this is a release ZIP, re-download it from GitHub ^(corrupt installer^).
    echo If this is the dev tree, make sure the cameraunlock-core submodule is checked out.
    exit /b 1
)
set "_SHIM_OUT=%TEMP%\cul-find-%RANDOM%-%RANDOM%.cmd"
set "_GIVEN_ARG="
if defined _GIVEN_PATH set "_GIVEN_ARG=-GivenPath "!_GIVEN_PATH!""
powershell -NoProfile -ExecutionPolicy Bypass -File "%_SHIM%" -GameId %GAME_ID% -OutFile "!_SHIM_OUT!" !_GIVEN_ARG!
set "_PS_EC=!errorlevel!"
if not "!_PS_EC!"=="0" (
    echo.
    echo ERROR: Could not resolve game install path ^(shim exit code !_PS_EC!^).
    echo Pass a path explicitly: uninstall.cmd "C:\path\to\game"
    echo.
    del "!_SHIM_OUT!" 2>nul
    exit /b 1
)
call "!_SHIM_OUT!"
del "!_SHIM_OUT!" 2>nul

echo Game found: "%GAME_PATH%"
echo.

:: -------- Game-running check --------
tasklist /fi "imagename eq %GAME_EXE%" 2>nul | findstr /i "%GAME_EXE%" >nul 2>&1
if not errorlevel 1 (
    echo ERROR: %GAME_DISPLAY_NAME% is currently running.
    echo Please close the game before uninstalling.
    echo.
    exit /b 1
)

:: -------- Compute DEPLOY_DIR per FRAMEWORK_TYPE --------
call :compute_deploy_dir
if errorlevel 1 exit /b 1

:: -------- Remove mod files (framework-aware) --------
if /i "%FRAMEWORK_TYPE%"=="None" (
    call :remove_shim_files
) else if /i "%FRAMEWORK_TYPE%"=="MonoCecil" (
    :: Cecil: restore backup THEN remove our DLLs from Managed/.
    call :remove_MonoCecil
    call :remove_mod_files_plain
    call :remove_managed_extras
) else (
    call :remove_mod_files_plain
)

:: -------- Decide whether to remove loader --------
set "REMOVE_LOADER=0"
if "!FORCE_FLAG!"=="1" set "REMOVE_LOADER=1"
if "!REMOVE_LOADER!"=="0" (
    if exist "%GAME_PATH%\%STATE_FILE%" (
        findstr /c:"installed_by_us" "%GAME_PATH%\%STATE_FILE%" 2>nul | findstr /c:"true" >nul 2>&1
        if not errorlevel 1 set "REMOVE_LOADER=1"
    )
)

if /i "%FRAMEWORK_TYPE%"=="None" (
    :: Shim-only: already handled in :remove_shim_files above (restores .backup).
    rem
) else if /i "%FRAMEWORK_TYPE%"=="MonoCecil" (
    :: Cecil: the backup restore IS the loader removal. Already done.
    rem
) else (
    if "!REMOVE_LOADER!"=="1" (
        echo.
        if "!FORCE_FLAG!"=="1" (
            echo Removing %FRAMEWORK_TYPE% ^(/force^)...
        ) else (
            echo Removing %FRAMEWORK_TYPE% ^(installed by this mod^)...
        )
        call :remove_%FRAMEWORK_TYPE%
    ) else (
        echo.
        echo %FRAMEWORK_TYPE% was not installed by this mod - leaving intact. Use /force to remove anyway.
    )
)

:: -------- Remove state file --------
if exist "%GAME_PATH%\%STATE_FILE%" (
    del "%GAME_PATH%\%STATE_FILE%"
    echo   Removed: state file
)

echo.
echo === Uninstall Complete ===
echo.
exit /b 0

:: ============================================
:: compute_deploy_dir: set DEPLOY_DIR based on FRAMEWORK_TYPE.
:: For ASILoader and None, DEPLOY_DIR is derived from the shim's
:: GAME_EXE_RELPATH (so nested-exe games like DL2 work).
:: ============================================
:compute_deploy_dir
if /i "%FRAMEWORK_TYPE%"=="BepInEx" (
    set "DEPLOY_DIR=%GAME_PATH%\BepInEx\plugins"
    exit /b 0
)
if /i "%FRAMEWORK_TYPE%"=="MelonLoader" (
    set "DEPLOY_DIR=%GAME_PATH%\Mods"
    exit /b 0
)
if /i "%FRAMEWORK_TYPE%"=="REFramework" (
    set "DEPLOY_DIR=%GAME_PATH%\reframework\plugins"
    exit /b 0
)
if /i "%FRAMEWORK_TYPE%"=="MonoCecil" (
    set "DEPLOY_DIR=%GAME_PATH%\%MANAGED_SUBFOLDER%"
    exit /b 0
)
if /i "%FRAMEWORK_TYPE%"=="ASILoader" (
    for %%i in ("%GAME_PATH%\%GAME_EXE_RELPATH%") do set "DEPLOY_DIR=%%~dpi"
    if "!DEPLOY_DIR:~-1!"=="\" set "DEPLOY_DIR=!DEPLOY_DIR:~0,-1!"
    exit /b 0
)
if /i "%FRAMEWORK_TYPE%"=="None" (
    for %%i in ("%GAME_PATH%\%GAME_EXE_RELPATH%") do set "DEPLOY_DIR=%%~dpi"
    if "!DEPLOY_DIR:~-1!"=="\" set "DEPLOY_DIR=!DEPLOY_DIR:~0,-1!"
    exit /b 0
)
echo ERROR: Unknown FRAMEWORK_TYPE "%FRAMEWORK_TYPE%" in uninstall CONFIG BLOCK.
exit /b 1

:: ============================================
:: Remove mod DLLs + legacy DLLs from DEPLOY_DIR (framework-generic).
:: ============================================
:remove_mod_files_plain
echo Removing mod files...
set "REMOVED=0"
for %%f in (%MOD_DLLS%) do (
    if exist "!DEPLOY_DIR!\%%f" (
        del "!DEPLOY_DIR!\%%f"
        echo   Removed: %%f
        set /a REMOVED+=1
    )
)
if defined LEGACY_DLLS (
    for %%f in (%LEGACY_DLLS%) do (
        if exist "!DEPLOY_DIR!\%%f" (
            del "!DEPLOY_DIR!\%%f"
            echo   Removed: %%f ^(legacy^)
            set /a REMOVED+=1
        )
    )
)
if "!REMOVED!"=="0" echo   No mod files found
exit /b 0

:: ============================================
:: Remove extra files Cecil mods leave in Managed/ (configs, logs, etc.).
:: ============================================
:remove_managed_extras
if not defined MANAGED_EXTRAS exit /b 0
for %%f in (%MANAGED_EXTRAS%) do (
    if exist "!DEPLOY_DIR!\%%f" (
        del "!DEPLOY_DIR!\%%f"
        echo   Removed: %%f
    )
)
exit /b 0

:: ============================================
:: Remove shim DLLs - restore <name>.backup if present so the user's
:: pre-mod state comes back. Also handles any LEGACY_DLLS list entries.
:: ============================================
:remove_shim_files
echo Removing shim files...
set "REMOVED=0"
for %%f in (%MOD_DLLS%) do (
    if exist "!DEPLOY_DIR!\%%f.backup" (
        if exist "!DEPLOY_DIR!\%%f" del /q "!DEPLOY_DIR!\%%f" >nul 2>&1
        move /y "!DEPLOY_DIR!\%%f.backup" "!DEPLOY_DIR!\%%f" >nul
        echo   Restored original %%f from backup
        set /a REMOVED+=1
    ) else (
        if exist "!DEPLOY_DIR!\%%f" (
            del "!DEPLOY_DIR!\%%f"
            echo   Removed: %%f ^(no backup was present^)
            set /a REMOVED+=1
        )
    )
)
if defined LEGACY_DLLS (
    for %%f in (%LEGACY_DLLS%) do (
        if exist "!DEPLOY_DIR!\%%f" (
            del "!DEPLOY_DIR!\%%f"
            echo   Removed: %%f ^(legacy^)
            set /a REMOVED+=1
        )
    )
)
if "!REMOVED!"=="0" echo   No shim files found
exit /b 0

:: ============================================
:: Remove BepInEx (regular and BepInExPack both land in the same layout).
:: ============================================
:remove_BepInEx
if exist "%GAME_PATH%\BepInEx" (
    rmdir /s /q "%GAME_PATH%\BepInEx"
    echo   Removed: BepInEx folder
)
for %%f in (winhttp.dll doorstop_config.ini .doorstop_version changelog.txt) do (
    if exist "%GAME_PATH%\%%f" (
        del "%GAME_PATH%\%%f"
        echo   Removed: %%f
    )
)
exit /b 0

:: ============================================
:: Remove MelonLoader. Only delete Mods/UserLibs/UserData if empty
:: (mod-file removal above may leave them clean; users with other
:: melon mods installed keep their data).
:: ============================================
:remove_MelonLoader
if exist "%GAME_PATH%\MelonLoader" (
    rmdir /s /q "%GAME_PATH%\MelonLoader"
    echo   Removed: MelonLoader folder
)
for %%f in (version.dll dobby.dll NOTICE.txt) do (
    if exist "%GAME_PATH%\%%f" (
        del "%GAME_PATH%\%%f"
        echo   Removed: %%f
    )
)
for %%d in (Mods UserLibs UserData) do (
    if exist "%GAME_PATH%\%%d" (
        dir /b /a "%GAME_PATH%\%%d" 2>nul | findstr /r /v "^$" >nul
        if errorlevel 1 (
            rmdir "%GAME_PATH%\%%d" 2>nul
            if not exist "%GAME_PATH%\%%d" echo   Removed: %%d\ ^(empty^)
        )
    )
)
exit /b 0

:: ============================================
:: Mono.Cecil: restore Assembly-CSharp.dll from the .original backup.
:: The mod DLLs in Managed/ are cleaned up separately by the plain loop.
:: ============================================
:remove_MonoCecil
set "MANAGED_PATH=%GAME_PATH%\%MANAGED_SUBFOLDER%"
set "ASSEMBLY_PATH=%MANAGED_PATH%\%ASSEMBLY_DLL%"
set "BACKUP_PATH=%ASSEMBLY_PATH%.original"
if exist "%BACKUP_PATH%" (
    copy /y "%BACKUP_PATH%" "%ASSEMBLY_PATH%" >nul
    del "%BACKUP_PATH%"
    echo   Restored: %ASSEMBLY_DLL% from backup
) else (
    echo   WARNING: no %ASSEMBLY_DLL%.original backup found.
    echo   Run Steam "Verify integrity of game files" if the game misbehaves.
)
exit /b 0

:: ============================================
:: Remove Ultimate ASI Loader from EXE_DIR.
:: ============================================
:remove_ASILoader
for %%i in ("%GAME_PATH%\%GAME_EXE_RELPATH%") do set "EXE_DIR=%%~dpi"
if "!EXE_DIR:~-1!"=="\" set "EXE_DIR=!EXE_DIR:~0,-1!"
for %%f in (%ASI_LOADER_NAME% winmm.dll dinput8.dll xinput1_3.dll) do (
    if exist "!EXE_DIR!\%%f" (
        del "!EXE_DIR!\%%f"
        echo   Removed: %%f
    )
)
exit /b 0

:: ============================================
:: Remove REFramework.
:: ============================================
:remove_REFramework
if exist "%GAME_PATH%\dinput8.dll" (
    del "%GAME_PATH%\dinput8.dll"
    echo   Removed: dinput8.dll
)
if exist "%GAME_PATH%\reframework" (
    rmdir /s /q "%GAME_PATH%\reframework"
    echo   Removed: reframework/
)
:: Loose files REFramework's zip drops at the game root: the revision marker,
:: plus VR runtime DLLs the install stripped for flatscreen mode (clean up any
:: an older install left behind) so uninstall returns the game to vanilla.
for %%f in (reframework_revision.txt openvr_api.dll openxr_loader.dll DELETE_OPENVR_API_DLL_IF_YOU_WANT_TO_USE_OPENXR) do (
    if exist "%GAME_PATH%\%%f" (
        del /q "%GAME_PATH%\%%f" >nul 2>&1
        echo   Removed: %%f
    )
)
exit /b 0

:: ============================================
:: Shim-only: no framework to remove beyond the shim DLL (handled already).
:: ============================================
:remove_None
exit /b 0
