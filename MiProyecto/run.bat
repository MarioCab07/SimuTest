@echo off
REM ──────────────────────────────────────────────────────────────────────
REM Compila y ejecuta tu proyecto SFML sin VS Code
REM ──────────────────────────────────────────────────────────────────────

REM 1) Asegúrate de que existe la carpeta build
if not exist build (
    mkdir build
)

REM 2) Invoca g++ para compilar
"C:\Dev\mingw64\bin\g++.exe" ^
    -std=c++17 ^
    -IC:\Dev\MiProyecto\src ^
    -IC:\Dev\SFML-2.5.1\include ^
    C:\Dev\MiProyecto\src\main.cpp ^
    -o C:\Dev\MiProyecto\build\MiProyecto.exe ^
    -LC:\Dev\SFML-2.5.1\lib ^
    -lsfml-graphics -lsfml-window -lsfml-system ^
    -lopengl32 -lgdi32 -lwinmm -lole32 -loleaut32 -luuid
if errorlevel 1 (
    echo.
    echo **********   ERROR de compilacion   **********
    pause
    exit /b 1
)

REM 3) Copia assets si no existe
if not exist build\assets (
    xcopy /E /I C:\Dev\MiProyecto\assets build\assets >nul
)

REM 4) Ejecuta el programa
echo.
echo **********   Ejecutando MiProyecto.exe   **********
build\MiProyecto.exe

REM 5) Mantén la ventana abierta para leer la salida
echo.
echo Presiona cualquier tecla para salir...
pause
