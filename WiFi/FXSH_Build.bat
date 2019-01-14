@echo off
rem Do not edit! This batch file is created by CASIO fx-9860G SDK.

if exist debug\*.obj  del debug\*.obj
if exist WIFI.G1A  del WIFI.G1A

cd debug
if exist FXADDINror.bin  del FXADDINror.bin
"D:\Program Files\CASIO\fx-9860G SDK\OS\SH\Bin\Hmake.exe" Addin.mak
cd ..
if not exist debug\FXADDINror.bin  goto error

"D:\Program Files\CASIO\fx-9860G SDK\Tools\MakeAddinHeader363.exe" "D:\Documents\GTR\Wifi\WiFi"
if not exist WIFI.G1A  goto error
echo Build has completed.
goto end

:error
echo Build was not successful.

:end

