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

@rem Do not invoke this directly; it is invoked by makedist*.bat
@rem usage: create_vessel_zip_files        XRVesselsDir        workdir   vesselName
@rem        e.g., create_vessel_zip_files  ..\..\..\XRVessels  2.0\work  DeltaGliderXR1

setlocal
set XRVesselsDir=%1
set workdir=%2
set vesselName=%3

@rem =============================================
@rem 32-bit
@rem =============================================
call :makezip x86
call :makezip x64
dir %version%
goto :eof

@rem %1 = x86 or x64
:makezip
set arch=%1
set x64binarySubdir=
if '%arch%' == 'x64' set x64binarySubdir=x64\

echo Building %arch%-bit %vesselName% zip files

@rem copy the DLL
call :copyfile "%XRVesselsDir%\%x64binarySubdir%release\%vesselName%.dll" %workdir%\Modules\*

@rem create the special DLL-only ZIP file (useful for unofficial releases such as a beta)
set zipfile=%version%\%vesselName%-%version%-dll-%arch%.zip
if exist %zipfile% del %zipfile%
call WinRar a -afzip -ep -m5 %zipfile% "%workdir%\Modules\%vesselName%.dll"

@rem create the ZIP distribution file
pushd %workdir%
set zipfile=..\%vesselName%-%version%-%arch%.zip
if exist %zipfile% del "%zipfile%"

call WinRar a -afzip -ep1 -m5 -r  "%zipfile%" *

popd
@echo Done with %arch% %vesselName% zip files.
goto :eof

rem -------------
rem Copy a file after verifying it exists
rem -------------
:copyfile
  set src=%1
  set dest=%2
  echo "Copying %src% -> %dest%"
  if not exist %src% goto not_found
  copy %src% %dest% /y
  goto :eof

:not_found
  echo ERROR: source file '%src%' does not exist
  pause
  goto :eof
