#!/bin/sh

# switch to release mode and init anew
build/bin/ferret -M RELEASE .

# full clean
build/bin/ferret -c

# build
build/bin/ferret -p 5 

cp -f bin/RELEASE/ferret     build/bin
cp -f bin/RELEASE/ferret_inc build/bin

tar zcvf ferret_build.tgz build/bin build/platform_$HOSTNAME.xml build/build_$HOSTNAME.properties build/ferret_dep.sh build/script_templ

exit 0

