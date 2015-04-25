@echo off
mkdir ..\..\build
pushd ..\..\build
cl -DHANDMADE_DEBUG=1 -DHANDMADE_INTERNAL=1 -FC -Zi ..\handmade\code\win32_handmade.cpp user32.lib gdi32.lib
popd

