#!/bin/bash
killall nest
args=$(<args.txt)
set -e
./whitgl/scripts/build.sh
(cd build/out/Nest.app/Contents/MacOS; ./nest hotreload $args "$@" &)
