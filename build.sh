#!/bin/bash
killall nest
args=$(<args.txt)
set -e
mkdir -p build
./whitgl/scripts/build.sh
(cd build/out/Nest.app/Contents/MacOS; ./nest hotreload $args "$@" &)
