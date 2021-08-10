@echo off
set version=%1
set vessel=XR1

if NOT '%version%' == '' GOTO version_ok
echo Usage: makedist%vessel% 2.0
popd
goto :eof

:version_ok

@rem NOTE: workdir MUST BE one level under version dir for zip file creation to work!
set workdir=%version%\work
set orbiterdir=..\..\..\Orbiter
set XRVesselsDir=..\..\..\XRVessels
set outdir=out\%vessel%

if not exist %outdir% mkdir %outdir%
pushd %outdir%

@rem whack the work dir if it exists; do NOT whack the version directory!  It may have other files in it!
if exist %workdir% rd /s /q %workdir%

@rem create directory structure
mkdir %workdir%
mkdir %workdir%\Images\Vessels
mkdir %workdir%\Config\Vessels
mkdir %workdir%\Doc
mkdir %workdir%\Meshes\DG-XR1
mkdir %workdir%\Modules
mkdir %workdir%\Scenarios\DG-XR1
mkdir %workdir%\Textures\DG-XR1

@rem copy FULL DISTRIBUTION files
echo Copying XR Flight Operations Manual and 'DeltaGliderXR1Prefs.cfg'
call :copyfile "%XRVesselsDir%\XR Flight Operations Manual.pdf"       %workdir%\Doc\*
call :copyfile %XRVesselsDir%\DeltaGliderXR1\DeltaGliderXR1Prefs.cfg  %workdir%\Config\*
call :copyfile %orbiterdir%\images\vessels\DeltagliderXR1.bmp         %workdir%\images\vessels\*
call :copyfile %orbiterdir%\config\XR1-EXAMPLE.xrcfg                  %workdir%\config\*
call :copyfile %orbiterdir%\config\vessels\DeltagliderXR1.cfg         %workdir%\config\vessels\*
call :copyfile %orbiterdir%\meshes\DG-XR1\*.msh                       %workdir%\meshes\DG-XR1\*
call :copyfile %XRVesselsDir%\release\DeltaGliderXR1.dll              %workdir%\Modules\*
call :copyfile %orbiterdir%\scenarios\DG-XR1\*.scn                    %workdir%\scenarios\DG-XR1\*
call :copyfile %orbiterdir%\textures\DG-XR1\*.dds                     %workdir%\textures\DG-XR1\*.dds

@rem create the special DLL-only ZIP file (useful for unofficial releases such as a beta)
set zipfile=%version%\DeltaGliderXR1-%version%-dll.zip
if exist %zipfile% del %zipfile%
call WinRar a -afzip -ep -m5 %zipfile% %workdir%\Modules\DeltaGliderXR1.dll

@rem create the ZIP distribution file
set zipfile=%version%\DeltaGliderXR1-%version%.zip
if exist %zipfile% del %zipfile%
pushd %workdir%
@rem THIS ASSUMES WORKDIR IS ONE LEVEL UNDER VERSION DIR
call WinRar a -afzip -ep1 -m5 -r  ..\DeltaGliderXR1-%version%.zip *
popd
@echo Done with ZIP.

@echo DONE
cd %version%
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