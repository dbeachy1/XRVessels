@echo off
set version=%1

if NOT '%version%' == '' GOTO version_ok
echo Usage: makedistXRVesselCtrlDemo 3.2
popd
goto :eof

:version_ok

set workdir=%version%\work
set orbiterdir=..\..\..\Orbiter
set XRVesselsDir=..\..\..\XRVessels
set outdir=out\XRVesselCtrlDemo

set XRSrcDir=%XRVesselsDir%\XRVesselCtrlDemo
set XRDestDir=%workdir%\Orbitersdk\XRVesselCtrlDemo

if not exist %outdir% mkdir %outdir%
pushd %outdir%

@rem whack the work dir if it exists; do NOT whack the version directory!  It may have other files in it!
if exist %workdir% rd /s /q %workdir%

@rem create directory structure
mkdir %workdir%
mkdir %workdir%\Doc
mkdir %workdir%\Modules\Plugin
mkdir %XRDestDir%

call :copyfile %orbiterdir%\doc\XRVesselCtrlDemo-readme.txt %workdir%\Doc\*
call :copyfile %XRSrcDir%\*.cpp                             %XRDestDir%\*
call :copyfile %XRSrcDir%\*.h                               %XRDestDir%\*
call :copyfile %XRSrcDir%\*.rc                              %XRDestDir%\*
call :copyfile %XRSrcDir%\altealogo2_small.bmp              %XRDestDir%\*
call :copyfile %XRSrcDir%\XRVesselCtrlDemo.sln              %XRDestDir%\*
call :copyfile %XRSrcDir%\XRVesselCtrlDemo.vcxproj          %XRDestDir%\*
call :copyfile %XRSrcDir%\XRVesselCtrlDemo.vcxproj.filters  %XRDestDir%\*

@rem create the 32-bit ZIP distribution file
@echo Creating 32-bit XRVesselCtrlDemo zip file
call :copyfile %XRVesselsDir%\Release\XRVesselCtrlDemo.dll  %workdir%\Modules\Plugin\*
pushd %workdir%
set zipfile=..\XRVesselCtrlDemo-%version%-x86.zip
if exist %zipfile% del %zipfile%

call WinRar a -afzip -ep1 -m5 -r  %zipfile% *
popd

@rem create the 64-bit ZIP distribution file
@echo Creating 64-bit XRVesselCtrlDemo zip file
call :copyfile %XRVesselsDir%\x64\Release\XRVesselCtrlDemo.dll  %workdir%\Modules\Plugin\* /y
pushd %workdir%
set zipfile=..\XRVesselCtrlDemo-%version%-x64.zip
if exist %zipfile% del %zipfile%

call WinRar a -afzip -ep1 -m5 -r  %zipfile% *
popd

@rem Done
dir "%version%"
popd
@echo Done with XRVesselCtrlDemo zip files.
goto :eof

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
  