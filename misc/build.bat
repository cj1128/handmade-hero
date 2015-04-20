@echo off
mkdir ..\..\build
pushd ..\..\build
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64
cl /Zi ..\handmade\code\win32_handmade.cpp user32.lib gdi32.lib
popd

