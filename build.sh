#!/bin/bash
killall ld37
args=$(<args.txt)
set -e
./whitgl/scripts/build.sh
(cd build/out/LD37_whitingjp.app/Contents/MacOS; ./ld37 hotreload $args "$@" &)
