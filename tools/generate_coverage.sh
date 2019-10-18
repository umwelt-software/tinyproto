#!/bin/sh

make EXTRA_CPPFLAGS="--coverage" check
lcov -t "tinyproto" -o ./bld/tinyproto.info -c -d ./src
genhtml -o ./bld/report ./bld/tinyproto.info
