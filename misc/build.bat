@echo off
IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
cl -FC -MT -nologo -Gm- -GR- -EHa- ^
  -Od -Oi -WX -W4 ^
  -wd4201 -wd4100 -wd4189 -wd4800 ^
  -DHANDMADE_DEBUG=1 -DHANDMADE_INTERNAL=1 ^
  -DHANDMADE_WIN32=1 ^
  -Z7 ^
  -Fmwin32_handmade.map ..\handmade\code\win32_handmade.cpp ^
  /link -opt:ref -subsystem:windows,5.1 ^
  user32.lib gdi32.lib
popd

