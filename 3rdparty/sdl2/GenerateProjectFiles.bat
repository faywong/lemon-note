@echo off

echo Building SDL2 project files:
echo    SDL2
echo    SDL2main
echo.

@echo on
call premake5\premake5.exe vs2019
@echo off

pause