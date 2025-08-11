#!/bin/bash

LLVM_FLAGS=`llvm-config \
        --cxxflags \
        --ldflags \
        --system-libs \
        --libs core orcjit native`

LLVM_FLAGS=$(
    echo $LLVM_FLAGS \
    | sed "s/-std=c++17/-std=c++23/g" \
)

SRCS=(
    ./ast.cpp
    ./context.cpp
    ./parser.cpp
    ./tokenizer.cpp
    ./main.cpp
)

clang++ \
    -g -O3 \
    "${SRCS[@]}" \
    $LLVM_FLAGS \
    -Xlinker --export-dynamic \
    -fstandalone-debug -fsanitize=address \
    -o main

./main
