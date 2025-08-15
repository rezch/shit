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

# SANITIZER="-fstandalone-debug -fsanitize=address"

clang++ \
    -g -O3 \
    "${SRCS[@]}" \
    $LLVM_FLAGS \
    -Xlinker --export-dynamic \
    "$SANITIZER" \
    -o main \
    "$@"

notify-send --urgency=low "Build done"

./main
