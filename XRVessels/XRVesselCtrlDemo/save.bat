@echo off
if '%1' == '' goto USAGE
set fout=XRVesselCtrlDemo_%DATE%_%1.zip
call jpack -createzip "e:\save\%fout%" -en -r .
pause
copy "e:\save\%fout%"  u:\save\*
dir u:\save /od

goto :eof

:USAGE
echo Usage: save #  (e.g., save 0)
goto :eof