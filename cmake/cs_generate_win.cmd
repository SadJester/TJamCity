@echo off
setlocal

set "REPO_DIR=%~dp0\.."

rem call cmake -S ./CitySimulator -B ./.cmake/cs -T "v143,host=x64,version=14.38.33130"
call cmake --preset win