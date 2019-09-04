@echo off
if not exist w:\build mkdir w:\build
pushd w:\build
cl -FC -WX -W4 -wd4201 -wd4100 -MT -Oi -Od -GR- -Gm- -EHa- -Zi -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1 -DHANDMADE_WIN32=1 w:\handmade\code\win32_handmade.cpp /link -subsystem:windows,5.1 /ignore:4010 user32.lib gdi32.lib
popd
