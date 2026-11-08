@echo off
rem Build tool for Windows: build.bat setup | mats | pack

where /q python || (echo Python not found. & exit /b 1)

python tool %*
