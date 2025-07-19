
default:
    just --list

alias b := build

build:
    mkdir -p build
    clang++ -g -Iinclude src/main.cpp -o ./build/main -std=c++2b -lspdlog -fno-exceptions -lfmt -g
    sudo setcap cap_net_admin+ep ./build/main

run: build
    ./build/main
