#!/bin/sh

mkdir -p build/bin

if [ ! -x build/bin/ferret_inc ]
then
    g++ -o build/bin/ferret_inc inc/src/*.cpp
    [ ! -x build/bin/ferret_inc ] && exit 1
fi

if [ ! -x build/bin/ferret ]
then
    g++ -o build/bin/ferret mcp/src/*.cpp -lrt
    [ ! -x build/bin/ferret ] && exit 1
fi

[ ! -f build/platform_$HOSTNAME.xml ] && cp build/platform_generic.xml build/platform_$HOSTNAME.xml

[ ! -f build/build_$HOSTNAME.properties ] && cp build/build_generic.properties build/build_$HOSTNAME.properties

rm -rf ferret_db
rm -rf build/temp/ferret/*

build/bin/ferret -M DEBUG --ignhdr .
[ ! -f ferret_db/build/ferret_files ] && exit 1 

build/bin/ferret -p 5

if [ -x bin/DEBUG/ferret ]
then
    cp -fv bin/DEBUG/ferret  build/bin
fi
