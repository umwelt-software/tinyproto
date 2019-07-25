#!/bin/sh

cd ../..

OUTPUTPATH=releases/arduino/tinyproto
OUTPUTZIP=releases/arduino/tinyproto.zip

rm -rf $OUTPUTPATH
rm -rf $OUTPUTZIP

mkdir -p $OUTPUTPATH/src
cp src/*.cpp $OUTPUTPATH/src
##cp src/*.c $OUTPUTPATH/src
##cp src/*.hpp $OUTPUTPATH/src
cp src/*.h $OUTPUTPATH/src
cp -R examples $OUTPUTPATH
cp -R src/proto $OUTPUTPATH/src
rm -rf $OUTPUTPATH/src/proto/hal/linux
rm -rf $OUTPUTPATH/src/proto/hal/mingw32
rm -rf $OUTPUTPATH/src/proto/hal/tiny_defines.h
mv $OUTPUTPATH/src/proto/hal/arduino/* $OUTPUTPATH/src/proto/hal/
rm -rf $OUTPUTPATH/src/proto/hal/arduino
cp library.json $OUTPUTPATH
cp library.properties $OUTPUTPATH
cp LICENSE $OUTPUTPATH
cp keywords.txt $OUTPUTPATH


(cd releases/arduino/; zip -r tinyproto.zip tinyproto)

echo "arduino package build ... [DONE]"
