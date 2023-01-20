# !/bin/bash

clang++ -emit-llvm -S -O0 -fno-discard-value-names student.cpp main.cpp
llvm-link -S student.ll main.ll -o target.ll
clang++ target.ll -o ./target