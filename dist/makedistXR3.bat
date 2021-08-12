@echo off
setlocal
set version=%1
set vessel=XR3

if NOT '%version%' == '' GOTO version_ok
echo Usage: makedist%vessel% 2.0
popd
goto :eof

:version_ok

@rem NOTE: workdir MUST BE one level under version dir for zip file creation to work!
set workdir=%version%\work
set orbiterdir=..\..\..\Orbiter
set XRVesselsDir=..\..\..\XRVessels
set vesselName=XR3Phoenix
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
mkdir %workdir%\Meshes\XR3Phoenix
mkdir %workdir%\Meshes\XRPayload
mkdir %workdir%\Modules
mkdir "%workdir%\Scenarios\XR3 Phoenix"
mkdir %workdir%\Textures\XR3Phoenix

@rem copy FULL DISTRIBUTION files
echo Copying XR Flight Operations Manual and 'XR3PhoenixPrefs.cfg'
echo NOT UPDATED YET: call :copyfile "..\..\XRVessels\XRVessels\XR Flight Operations Manual.pdf" %workdir%\Doc\*
call :copyfile %XRVesselsDir%\XR3Phoenix\XR3PhoenixPrefs.cfg                %workdir%\Config\*
REM LATER: call :copyfile %orbiterdir%\images\vessels\XR3Phoenix.bmp        %workdir%\images\vessels\*
@rem NOT CREATED YET: call :copyfile %orbiterdir%\config\XR3-EXAMPLE.xrcfg  %workdir%\config\*
call :copyfile %orbiterdir%\config\vessels\XR3Phoenix.cfg                   %workdir%\config\vessels\*

call :copyfile %orbiterdir%\meshes\XR3Phoenix\*.msh                 %workdir%\meshes\XR3Phoenix\*
call :copyfile %orbiterdir%\meshes\XRPayloadBay.msh                 %workdir%\meshes\*
call :copyfile "%orbiterdir%\scenarios\XR3 Phoenix\*.scn"          "%workdir%\scenarios\XR3 Phoenix\*"
call :copyfile %orbiterdir%\textures\XR3Phoenix\*.dds               %workdir%\textures\XR3Phoenix\*

@rem create the x86 and x64 zip files
call ..\..\create_vessel_zip_files.bat %XRVesselsDir% %workdir% %vesselName%

popd
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
  