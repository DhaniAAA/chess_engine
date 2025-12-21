@echo off
REM ============================================================================
REM Gauntlet Testing Script for Chess Engine
REM Tests against multiple opponent engines
REM ============================================================================

echo ============================================
echo  Chess Engine Gauntlet Testing Script
echo ============================================
echo.

REM Configuration
set ENGINE_TEST=output\main.exe
set HASH=64
set THREADS=1
set GAMES_PER_OPPONENT=100
set TC=5+0.05

REM Opponent engines (add your opponents here)
set OPPONENTS=engines\opponent1.exe engines\opponent2.exe engines\opponent3.exe

echo Configuration:
echo   Tested Engine: %ENGINE_TEST%
echo   Games per opponent: %GAMES_PER_OPPONENT%
echo   Time Control: %TC%
echo.

REM Check if main engine exists
if not exist "%ENGINE_TEST%" (
    echo ERROR: Engine not found: %ENGINE_TEST%
    echo Please build the engine first with 'make'
    pause
    exit /b 1
)

echo.
echo To run gauntlet test, install cutechess-cli and run:
echo.
echo cutechess-cli ^
echo     -engine name="Tested" cmd="%ENGINE_TEST%" option.Hash=%HASH% ^
echo     -engine name="Opponent" cmd="OPPONENT_PATH" option.Hash=%HASH% ^
echo     -each proto=uci tc=%TC% ^
echo     -games %GAMES_PER_OPPONENT% ^
echo     -repeat -recover ^
echo     -draw movenumber=50 movecount=8 score=10 ^
echo     -resign movecount=3 score=1000
echo.

pause
