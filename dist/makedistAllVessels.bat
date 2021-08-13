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

if NOT '%version%' == '' GOTO version_ok
echo Usage: makedistAllVessels 2.0-RC1
popd
goto :eof

:version_ok
call makedistXR1 %version%
call makedistXR2 %version%
@REM once vessel is ready and version is in sync with other XR vessels: call makedistXR3 %version%
call makedistXR5 %version%
