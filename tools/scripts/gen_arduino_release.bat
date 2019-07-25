@echo off

chdir ..\..
rmdir /S /Q releases
mkdir releases\arduino\TinyProto\src

xcopy /Q /Y /E examples releases\arduino\TinyProto\examples\

xcopy /Q /Y /E src\*.cpp releases\arduino\TinyProto\src\
xcopy /Q /Y /E src\*.h releases\arduino\TinyProto\src\
xcopy /Q /Y /E src\proto releases\arduino\TinyProto\src\proto\

rmdir /S /Q releases\arduino\TinyProto\src\proto\hal\linux
rmdir /S /Q releases\arduino\TinyProto\src\proto\hal\mingw32
del releases\arduino\TinyProto\src\proto\hal\tiny_defines.h
move releases\arduino\TinyProto\src\proto\hal\arduino\* releases\arduino\TinyProto\src\proto\hal\ 
rmdir /S /Q releases\arduino\TinyProto\src\proto\hal\arduino

copy library.json releases\arduino\TinyProto\
copy library.properties releases\arduino\TinyProto\
copy LICENSE releases\arduino\TinyProto\
copy keywords.txt releases\arduino\TinyProto\

echo "arduino package build ... [DONE]"
