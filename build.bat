@echo off
rem usage: build.bat setup ^| check ^| build [--no-zip]
where python >nul 2>nul || (echo Python not found. & exit /b 1)
python tool %*
