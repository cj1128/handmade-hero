@echo off
mkdir w:\build
pushd w:\build
cl -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1 -Zi w:\handmade\code\win32_handmade.cpp user32.lib gdi32.lib
popd
