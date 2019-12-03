@echo off
echo STATICS FOUND
findstr -s -n -i -l "static" *.h *.cpp

echo ------
echo ------
echo GLOBALS FOUND
findstr -s -n -i -l "global_variable" *.h *.cpp
findstr -s -n -i -l "local_persist" *.h *.cpp
