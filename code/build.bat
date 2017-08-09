@echo off
mkdir ..\build
pushd ..\build
cl -nologo -Zi ..\code\bcoder_win32.cpp /DDEBUG gdi32.lib user32.lib
popd
