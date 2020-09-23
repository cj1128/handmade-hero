@echo off
rem Create a directory e.g. `casey` and put all casey's code into a subdirectory
rem called `archive`, put this script at the same level as archive.
rem `.\build_casey [day number]` to build casey's code

if NOT "%1" == "" goto :ok
echo error: must provide the number
exit /b 1

:ok
set num=000%1
set num=%num:~-3%
set folder=handmade_hero_day_%num%_source

set CommonCompilerFlags=-MTd -nologo -fp:fast -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4244 -wd4456 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 -FC -Z7
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

REM TODO - can we just build both with one exe?

IF NOT EXIST .\build mkdir .\build
pushd .\build

REM 32-bit build
REM cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64-bit build
del *.pdb > NUL 2> NUL
REM Optimization switches /O2
echo WAITING FOR PDB > lock.tmp
cl %CommonCompilerFlags% ..\archive\%folder%\code\handmade.cpp -Fmhandmade.map -LD /link -incremental:no -opt:ref -PDB:handmade_%random%.pdb -EXPORT:GameGetSoundSamples -EXPORT:GameUpdateAndRender
del lock.tmp
cl %CommonCompilerFlags% ..\archive\%folder%\code\win32_handmade.cpp -Fmwin32_handmade.map /link %CommonLinkerFlags%
popd
