@rem ##
@rem #  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
@rem #  Copyright (C) 2006-2021 Douglas Beachy
@rem #
@rem #  This program is free software: you can redistribute it and/or modify
@rem #  it under the terms of the GNU General Public License as published by
@rem #  the Free Software Foundation, either version 3 of the License, or
@rem #  any later version.
@rem #
@rem #  This program is distributed in the hope that it will be useful,
@rem #  but WITHOUT ANY WARRANTY; without even the implied warranty of
@rem #  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
@rem #  GNU General Public License for more details.
@rem #
@rem #  You should have received a copy of the GNU General Public License
@rem #  along with this program.  If not, see <https://www.gnu.org/licenses/>.
@rem #
@rem #  Email: mailto:doug.beachy@outlook.com
@rem #  Web: https://www.alteaaerospace.com
@rem ##

@echo off
setlocal
set version=%1
set vessel=XR2

if NOT '%version%' == '' GOTO version_ok
echo Usage: makedist%vessel% 2.0
popd
goto :eof

:version_ok

@rem NOTE: workdir MUST BE one level under version dir for zip file creation to work!
set workdir=%version%\work
set orbiterdir=..\..\..\Orbiter
set XRVesselsDir=..\..\..\XRVessels
set vesselName=XR2Ravenstar
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
mkdir %workdir%\Meshes\XR2Ravenstar
mkdir %workdir%\Modules
mkdir "%workdir%\Scenarios\XR2 Ravenstar"
mkdir %workdir%\Textures\XR2Ravenstar

@rem copy FULL DISTRIBUTION files
echo Copying XR Flight Operations Manual and 'XR2RavenstarPrefs.cfg'
call :copyfile "%XRVesselsDir%\XR Flight Operations Manual.pdf"           %workdir%\Doc\*
call :copyfile %XRVesselsDir%\XR2Ravenstar\XR2RavenstarPrefs.cfg          %workdir%\Config\*
call :copyfile %orbiterdir%\images\vessels\XR2Ravenstar.bmp               %workdir%\images\vessels\*
call :copyfile %orbiterdir%\config\XR2-EXAMPLE.xrcfg                      %workdir%\config\*
call :copyfile %orbiterdir%\config\XR2-PhobosDeimosPayloadMission.xrcfg   %workdir%\config\*
call :copyfile %orbiterdir%\config\vessels\XR2Ravenstar.cfg               %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\XRPayloadBay.cfg               %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\XR2turbopackLee.cfg            %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\XR2turbopackKara.cfg           %workdir%\config\vessels\*

call :copyfile %orbiterdir%\config\vessels\XR2PayloadCHM.cfg              %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\XR2PayloadLOX.cfg              %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\XR2PayloadLOX_Half.cfg         %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\XR2PayloadEmptyLOX.cfg         %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\XR2PayloadMainFuel.cfg         %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\XR2PayloadSCRAMFuel.cfg        %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\PayloadThumbnailLOX.bmp        %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\PayloadThumbnailMainFuel.bmp   %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\PayloadThumbnailSCRAMFuel.bmp  %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\PayloadThumbnailLOX_Half.bmp   %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\PayloadThumbnailCHM.bmp        %workdir%\config\vessels\*
call :copyfile %orbiterdir%\config\vessels\Altea_Default_Payload_Thumbnail.bmp    %workdir%\config\vessels\*

call :copyfile %orbiterdir%\meshes\XRPayloadBay.msh                       %workdir%\meshes\*

@rem these two sets of files, although free, cannot be open-source due to copyright
call :copyfile "%ORBITER_ROOT_RELEASE_X64%\meshes\XR2Ravenstar\*.msh"     %workdir%\meshes\XR2Ravenstar\*
set src=%ORBITER_ROOT_RELEASE_X64%\textures\XR2Ravenstar\*.dds
if not exist "%src%" goto :not_found
xcopy "%ORBITER_ROOT_RELEASE_X64%\textures\XR2Ravenstar\*.dds"            %workdir%\textures\XR2Ravenstar\* /s

xcopy "%orbiterdir%\scenarios\XR2 Ravenstar\*.scn"                        "%workdir%\scenarios\XR2 Ravenstar\*" /s

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
  