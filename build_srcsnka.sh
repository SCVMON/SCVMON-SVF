# !/bin/bash
set -x

cd Release-build
make Svf -j 4
make srcsnka -j 4
cd ..
