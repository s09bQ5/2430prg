@echo off
set DDK_HOME=c:\WinDDK
set DDK_VER=6000
set CURDIR=%CD%
%DDK_HOME%\%DDK_VER%\bin\setenv.bat %DDK_HOME%\%DDK_VER% chk AMD64
cd %CURDIR%
