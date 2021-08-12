@echo off
setlocal
set version=%1

if NOT '%version%' == '' GOTO version_ok
echo Usage: makedistAllVessels 2.0-RC1
popd
goto :eof

:version_ok
call makedistXR1 %version%
call makedistXR2 %version%
@REM once vessel is ready and version is in sync with other XR vessels: call makedistXR3 %version%
call makedistXR5 %version%
