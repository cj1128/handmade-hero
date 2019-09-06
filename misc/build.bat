@echo off
if not exist w:\build mkdir w:\build
pushd w:\build

set CommonCompilerFlags=-FC -WX -W4 -wd4201 -wd4100 -MT -Oi -Od -GR- -Gm- -EHa- -Zi -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1 -DHANDMADE_WIN32=1 -Fmwin32_handmade.map
set CommonLinkerFlags=-opt:ref user32.lib gdi32.lib

rem 32-bit build
rem cl %CommonCompilerFlags% w:\handmade\code\win32_handmade.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

rem 64-bit build
cl %CommonCompilerFlags% w:\handmade\code\win32_handmade.cpp /link %CommonLinkerFlags%
popd
