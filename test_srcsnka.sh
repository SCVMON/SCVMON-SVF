# !/bin/bash
set -x

cd Release-build
make Svf -j 4
make srcsnka -j 4
cd ..
. ./setup.sh

srcsnka ./target/target.ll --source-file=./target/sources.txt --sink-file=./target/sinks.txt --include-file=./target/includes.txt --add-edge-file=./target/add_edge.txt -stat=false --debug-only=pathchecker