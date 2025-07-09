
default:
    just --list

alias b := build

build:
    mkdir -p build
    clang++ -g -Iinclude src/main.cpp -o ./build/main -std=gnu++2b -lspdlog -fno-exceptions -lfmt
    sudo setcap cap_net_admin+ep ./build/main

run: build
    ./build/main
