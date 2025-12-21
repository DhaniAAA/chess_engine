@echo off
REM ============================================================================
REM SPRT Testing Script for Chess Engine
REM Uses cutechess-cli for tournament management
REM ============================================================================

echo ============================================
echo  Chess Engine SPRT Testing Script
echo ============================================
echo.

REM Configuration
set ENGINE_NEW=output\main.exe
set ENGINE_BASE=output\main_baseline.exe
set HASH=64
set THREADS=1
set GAMES=1000
set TC=10+0.1
set BOOK=tests\book.pgn
set OUTPUT_PGN=tests\sprt_results.pgn

REM SPRT Parameters
REM H0: Elo = 0 (no improvement)
REM H1: Elo = 5 (5 ELO improvement)
set ELO0=0
set ELO1=5
set ALPHA=0.05
set BETA=0.05

echo Configuration:
echo   New Engine: %ENGINE_NEW%
echo   Baseline: %ENGINE_BASE%
echo   Hash: %HASH% MB
echo   Games: %GAMES%
echo   Time Control: %TC%
echo   SPRT Bounds: [%ELO0%, %ELO1%]
echo.

REM Check if engines exist
if not exist "%ENGINE_NEW%" (
    echo ERROR: New engine not found: %ENGINE_NEW%
    echo Please build the engine first with 'make'
    goto :error
)

if not exist "%ENGINE_BASE%" (
    echo WARNING: Baseline engine not found: %ENGINE_BASE%
    echo Creating a copy of current engine as baseline...
    copy /Y "%ENGINE_NEW%" "%ENGINE_BASE%"
)

REM Check for cutechess-cli
where cutechess-cli >nul 2>&1
if errorlevel 1 (
    echo WARNING: cutechess-cli not found in PATH
    echo.
    echo Please install cutechess-cli:
    echo   1. Download from: https://github.com/cutechess/cutechess/releases
    echo   2. Add to PATH or place in this directory
    echo.
    echo Alternatively, use fastchess:
    echo   Download from: https://github.com/Disservin/fastchess/releases
    goto :manual_command
)

echo Starting SPRT test...
echo.

cutechess-cli ^
    -engine name="New" cmd="%ENGINE_NEW%" option.Hash=%HASH% option.Threads=%THREADS% ^
    -engine name="Baseline" cmd="%ENGINE_BASE%" option.Hash=%HASH% option.Threads=%THREADS% ^
    -each proto=uci tc=%TC% ^
    -sprt elo0=%ELO0% elo1=%ELO1% alpha=%ALPHA% beta=%BETA% ^
    -games %GAMES% ^
    -rounds %GAMES% ^
    -repeat ^
    -recover ^
    -draw movenumber=50 movecount=8 score=10 ^
    -resign movecount=3 score=1000 ^
    -pgnout "%OUTPUT_PGN%" ^
    -concurrency %THREADS%

echo.
echo Test completed! Results saved to %OUTPUT_PGN%
goto :end

:manual_command
echo.
echo Manual command to run (copy and paste when cutechess-cli is available):
echo.
echo cutechess-cli -engine name="New" cmd="%ENGINE_NEW%" option.Hash=%HASH% -engine name="Baseline" cmd="%ENGINE_BASE%" option.Hash=%HASH% -each proto=uci tc=%TC% -sprt elo0=%ELO0% elo1=%ELO1% alpha=%ALPHA% beta=%BETA% -games %GAMES% -repeat -recover -draw movenumber=50 movecount=8 score=10 -resign movecount=3 score=1000
echo.
goto :end

:error
echo.
echo Script terminated with errors.
exit /b 1

:end
echo.
pause
