#!/bin/bash

LLVM_FLAGS=`llvm-config \
        --cxxflags \
        --ldflags \
        --system-libs \
        --libs core orcjit native`

LLVM_FLAGS=$( echo $LLVM_FLAGS \
    | sed "s/-fno-exceptions //g" \
    | sed "s/-std=c++17/-std=c++23/g" \
)

clang++ \
    -g main.cpp tokenizer.cpp \
    -fstandalone-debug -fsanitize=address \
    -std=c++23 \
   $LLVM_FLAGS \
    -O0 -o main
