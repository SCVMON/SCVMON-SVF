# !/bin/bash

clang++ -c student.cpp
clang++ main.cpp -o ./target student.o
