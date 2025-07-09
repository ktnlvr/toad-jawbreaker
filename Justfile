
default:
    just --list

alias b := build

build:
    mkdir -p build
    clang++ -g -Iinclude src/main.cpp -o ./build/main
    sudo setcap cap_net_admin+ep ./build/main

run: build
    ./build/main && (sleep 1 && sudo ip link set toad up)
    