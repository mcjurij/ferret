#!/bin/sh

mkdir build/bin

g++ -o build/bin/ferret_inc inc/src/*.cpp
[ ! -x build/bin/ferret_inc ] && exit 1

g++ -o build/bin/ferret mcp/src/*.cpp -lrt
[ ! -x build/bin/ferret ] && exit 1

[ ! -f build/platform_$HOSTNAME.xml ] && cp build/platform_heron64.xml  build/platform_$HOSTNAME.xml

[ ! -f build/build_$HOSTNAME.properties ] && cp build/build_heron64.properties build/build_$HOSTNAME.properties

rm -rf ferret_db

build/bin/ferret --init --ignhdr .
[ ! -f ferret_db/build/ferret_files ] && exit 1 

build/bin/ferret -p 5
