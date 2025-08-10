#!/bin/bash

LLVM_FLAGS=`llvm-config \
        --cxxflags \
        --ldflags \
        --system-libs \
        --libs core orcjit native`

LLVM_FLAGS=$( echo $LLVM_FLAGS \
    | sed "s/-std=c++17/-std=c++23/g" \
)

clang++ \
    -g -O3 main.cpp tokenizer.cpp \
    -fstandalone-debug -fsanitize=address \
   $LLVM_FLAGS \
    -o main
