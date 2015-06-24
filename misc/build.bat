@echo off
IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

set CommonCompilerFlags= -FC -MTd -nologo -Gm- -GR- -EHa- ^
  -Od -Oi -WX -W4 ^
  -wd4201 -wd4100 -wd4189 -wd4800 -wd4505^
  -DHANDMADE_DEBUG=1 -DHANDMADE_INTERNAL=1 ^
  -DHANDMADE_WIN32=1 ^
  -Z7
set CommonLinkerFlags= -opt:ref ^
    -incremental:no ^
    user32.lib gdi32.lib winmm.lib

set datetimef=%date:~-4%_%date:~3,2%_%date:~0,2%__%time:~0,2%_%time:~3,2%_%time:~6,2%
set datetimef=%datetimef: =_%

REM 32-bit build
REM cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp /link %CommonLinkerFlags% -subsystem:windows,5.1

REM 64-bit build
del *.pdb > NUL 2> NUL
cl %CommonCompilerFlags% ..\handmade\code\handmade.cpp -Fmhandmade.map /LD /link %CommonLinkerFlags% -PDB:handmade_%datetimef%.pdb -EXPORT:GameUpdateAudio -EXPORT:GameUpdateVideo
cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp -Fmwin32_handmade.map /link %CommonLinkerFlags%
popd

