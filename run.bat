��@echo off
cd %~dp0
balls.exe 100 5
if %errorlevel% neq 0 (
    echo The program failed to run correctly.
else (
    echo The program ran successfully.
)
pause
