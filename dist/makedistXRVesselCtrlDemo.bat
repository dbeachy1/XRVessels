@echo off
set version=%1

if NOT '%version%' == '' GOTO version_ok
echo Usage: makedistXRVesselCtrlDemo 2.0
popd
goto :eof

:version_ok

set workdir=out\XRVesselCtrlDemo\%version%\work
set orbiterdir=..\..\..\Orbiter
set XRVesselsDir=..\XRVessels

set XRSrcDir=%XRVesselsDir%\XRVesselCtrlDemo
set XRDestDir=%workdir%\Orbitersdk\XRVesselCtrlDemo

@rem whack the work dir if it exists; do NOT whack the version directory!  It may have other files in it!
if exist %workdir% rd /s %workdir%

@rem create directory structure
mkdir %workdir%
mkdir %workdir%\Doc
mkdir %workdir%\Modules\Plugin
mkdir %XRDestDir%

call :copyfile %orbiterdir%\doc\XRVesselCtrlDemo-readme.txt %workdir%\Doc\*
call :copyfile %XRVesselsDir%\Release\XRVesselCtrlDemo.dll  %workdir%\Modules\Plugin\*
call :copyfile %XRSrcDir%\*.cpp                             %XRDestDir%\*
call :copyfile %XRSrcDir%\*.h                               %XRDestDir%\*
call :copyfile %XRSrcDir%\*.rc                              %XRDestDir%\*
call :copyfile %XRSrcDir%\altealogo2_small.bmp              %XRDestDir%\*
call :copyfile %XRSrcDir%\XRVesselCtrlDemo.sln              %XRDestDir%\*
call :copyfile %XRSrcDir%\XRVesselCtrlDemo.vcxproj          %XRDestDir%\*
call :copyfile %XRSrcDir%\XRVesselCtrlDemo.vcxproj.filters  %XRDestDir%\*

@rem create the ZIP distribution file
set zipfile=%version%\XRVesselCtrlDemo-%version%.zip
if exist %zipfile% del %zipfile%
pushd %workdir%
@rem THIS ASSUMES WORKDIR IS ONE LEVEL UNDER VERSION DIR
call WinRar a -afzip -ep1 -m5 -r  ..\XRVesselCtrlDemo-%version%.zip *
popd
pause

@echo DONE
cd out\XRVesselCtrlDemo\%version%
dir
@goto :eof

rem -------------
rem Copy a file after verifying it exists
rem -------------
:copyfile
  set src=%1
  set dest=%2
  echo "Copying %src% -> %dest%"
  if not exist %src% goto not_found
  copy %src% %dest%
  goto :eof

:not_found
  echo ERROR: source file '%src%' does not exist
  pause
  goto :eof
  