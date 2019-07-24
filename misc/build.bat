@echo off
mkdir w:\build
pushd w:\build
cl -Zi w:\handmade\code\win32_handmade.cpp user32.lib gdi32.lib
popd
