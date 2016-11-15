#!/bin/sh

# switch to release mode and init anew
build/bin/ferret -M RELEASE .

# full clean
build/bin/ferret -c

# build
build/bin/ferret

cp -f bin/RELEASE/ferret     build/bin
cp -f bin/RELEASE/ferret_inc build/bin


mv -f build/temp/ferret  /tmp/
tar zcvf ferret_build.tgz build
mv -f /tmp/ferret build/temp/

exit 0

