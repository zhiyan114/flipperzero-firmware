@echo off
call "%~dp0scripts\toolchain\fbtenv.cmd" env || exit /b

set SCONS_EP=python -m SCons



set "SCONS_DEFAULT_FLAGS=--warn=target-not-built"

if not defined FBT_VERBOSE (
    set "SCONS_DEFAULT_FLAGS=%SCONS_DEFAULT_FLAGS% -Q"
)

%SCONS_EP% %SCONS_DEFAULT_FLAGS% %*
