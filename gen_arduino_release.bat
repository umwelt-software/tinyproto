@echo off

rmdir /S /Q releases\arduino\TinyProto\src
rmdir /S /Q releases\arduino\TinyProto\examples
rmdir /S /Q releases\arduino\TinyProto\tools
mkdir releases\arduino\TinyProto\src\proto
xcopy /Q /Y /E src\arduino\* releases\arduino\TinyProto\
del releases\arduino\TinyProto\library.properties.in
xcopy /Q /Y /E src\lib\* releases\arduino\TinyProto\src\proto\
xcopy /Q /Y /E tools releases\arduino\TinyProto\tools\
copy inc\*.h releases\arduino\TinyProto\src\proto\
copy inc\serial_api.h releases\arduino\TinyProto\src\proto\serial\
copy inc\os\arduino\*.h releases\arduino\TinyProto\src\proto\

echo "arduino package build ... [DONE]"
