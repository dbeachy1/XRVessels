@echo off
setlocal
set version=%1
set vessel=XR5

if NOT '%version%' == '' GOTO version_ok
echo Usage: makedist%vessel% 2.0
popd
goto :eof

:version_ok

@rem NOTE: workdir MUST BE one level under version dir for zip file creation to work!
set workdir=%version%\work
set orbiterdir=..\..\..\Orbiter
set XRVesselsDir=..\..\..\XRVessels
set vesselName=XR5Vanguard
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
mkdir %workdir%\Meshes\XR5Vanguard
mkdir %workdir%\Meshes\XRPayload
mkdir %workdir%\Modules
mkdir "%workdir%\Scenarios\XR5 Vanguard"
mkdir %workdir%\Textures\XR5Vanguard
mkdir %workdir%\Textures\XRPayload
@rem NOT FOR XR5: mkdir %workdir%\Orbitersdk\include

@rem copy FULL DISTRIBUTION files
echo Copying XR Flight Operations Manual and 'XR5VanguardPrefs.cfg'
call :copyfile "%XRVesselsDir%\XR Flight Operations Manual.pdf"      %workdir%\Doc\*
call :copyfile %XRVesselsDir%\XR5Vanguard\XR5VanguardPrefs.cfg       %workdir%\Config\*
call :copyfile "%orbiterdir%\Doc\XR5-ER Extended Range Payload Systems 1.txt"      %workdir%\Doc\*
REM LATER: call :copyfile %orbiterdir%\images\vessels\XR5Vanguard.bmp        %workdir%\images\vessels\*
call :copyfile %orbiterdir%\config\XR5-EXAMPLE.xrcfg                 %workdir%\config\*
call :copyfile %orbiterdir%\config\vessels\XR5Vanguard.cfg           %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\XRPayloadBay.cfg          %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\XRParts.cfg               %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\XRPayloadTest.cfg         %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\XRPayload_sc_40.cfg       %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\AIA_Logistics.cfg         %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\XR1_Spare_Parts_Thumbnail.bmp          %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\Altea_Default_Payload_Thumbnail.bmp    %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\AIAlogistics.bmp                       %workdir%\config\vessels\*

@rem ********************** These next two lines copy the custom XR5 payload .cfg and .bmp files from Carmen A: XR5-ER Payload System 1.1
call :copyfile %orbiterdir%\config\vessels\CSA_XR5_ER_*.cfg          %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\CSA_XR5_ER_*.bmp          %workdir%\config\vessels\*

call :copyfile %orbiterdir%\meshes\XR5Vanguard\*.msh                 %workdir%\meshes\XR5Vanguard\*
call :copyfile %orbiterdir%\meshes\XRPayloadBay.msh                  %workdir%\meshes\*
call :copyfile %orbiterdir%\meshes\XRPayload\*.msh                   %workdir%\meshes\XRPayload\*
call :copyfile "%orbiterdir%\scenarios\XR5 Vanguard\*.scn"           "%workdir%\scenarios\XR5 Vanguard\*"
call :copyfile %orbiterdir%\textures\XR5Vanguard\*.dds               %workdir%\textures\XR5Vanguard\*
call :copyfile %orbiterdir%\textures\XRPayload\*.dds                 %workdir%\textures\XRPayload\*

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
  