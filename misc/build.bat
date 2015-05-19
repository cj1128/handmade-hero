@echo off
IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

set CommonCompilerFlags= -FC -MT -nologo -Gm- -GR- -EHa- ^
  -Od -Oi -WX -W4 ^
  -wd4201 -wd4100 -wd4189 -wd4800 ^
  -DHANDMADE_DEBUG=1 -DHANDMADE_INTERNAL=1 ^
  -DHANDMADE_WIN32=1 ^
  -Z7 ^
  -Fmwin32_handmade.map ^
  user32.lib gdi32.lib winmm.lib
set CommonLinkerFlags= -opt:ref

REM 32-bit build
REM cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp /link %CommonLinkerFlags% -subsystem:windows,5.1

REM 64-bit build
cl %CommonCompilerFlags% ..\handmade\code\handmade.cpp /LD /link -incremental:no -opt:ref
cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp /link %CommonLinkerFlags%
popd

