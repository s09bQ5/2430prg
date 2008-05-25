@echo off
set DDK_HOME=c:\WinDDK
set DDK_VER=6000
set CURDIR=%CD%
echo Compiling project
build -ceZ
echo Copying CC2430Cable.inf
copy %CURDIR%\objchk_wlh_amd64\amd64\CC2430Cable.inf %CURDIR%\package
echo Copying CC2430Cable.sys
copy %CURDIR%\objchk_wlh_amd64\amd64\CC2430Cable.sys %CURDIR%\package
cd %CURDIR%\package
echo Removing old driver
%DDK_HOME%\%DDK_VER%\tools\devcon\amd64\devcon.exe remove CC2430Cable.inf root\CC2430Cable
echo Signing driver file CC2430Cable.sys
signtool sign /v /s OtavioEngBr /n Otavio.eng.br(Test) /t http://timestamp.verisign.com/scripts/timestamp.dll CC2430Cable.sys
echo Creating catalog file
makecat /v CC2430Cable.cdf
echo Signing catalog file CC2430Cable.cat
signtool sign /v /s OtavioEngBr /n Otavio.eng.br(Test) /t http://timestamp.verisign.com/scripts/timestamp.dll CC2430Cable.cat
echo Deploying driver
%DDK_HOME%\%DDK_VER%\tools\devcon\amd64\devcon.exe install CC2430Cable.inf root\CC2430Cable
cd %CURDIR%