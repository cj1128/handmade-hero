@echo off
mkdir ..\..\build
pushd ..\..\build
cl /Zi /FC ..\handmade\code\win32_handmade.cpp user32.lib gdi32.lib
popd

