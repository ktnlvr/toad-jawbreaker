
default:
    just --list

alias b := build

build:
    mkdir -p build
    clang++ -g -Iinclude src/main.cpp -o ./build/main -std=c++2b -lspdlog -lfmt -g -luring

run: build
    ./build/main

alias ds := docs-serve
docs-serve:
    mkdocs serve

alias db := docs-build
docs-build:
    mkdir -p ./build/
    mkdocs build -d build/docs
