@rem ##
@rem #  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
@rem #  Copyright (C) 2006-2021 Douglas Beachy
@rem #
@rem #  This program is free software: you can redistribute it and/or modify
@rem #  it under the terms of the GNU General Public License as published by
@rem #  the Free Software Foundation, either version 3 of the License, or
@rem #  (at your option) any later version.
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
set vesselName=DeltaGliderXR1
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
call :copyfile %orbiterdir%\scenarios\DG-XR1\*.scn                    %workdir%\scenarios\DG-XR1\*
call :copyfile %orbiterdir%\textures\DG-XR1\*.dds                     %workdir%\textures\DG-XR1\*.dds

@rem create the x86 and x64 zip files
call ..\..\create_vessel_zip_files.bat %XRVesselsDir% %workdir% %vesselName%

popd
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