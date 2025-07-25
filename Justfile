
default:
    just --list

alias b := build
build:
    mkdir -p build
    clang++ -g -Iinclude src/main.cpp -o ./build/main -std=c++2b -lspdlog -lfmt -g -luring

run: build
    ./build/main

scaffold-star-run nodes='2': 
    mkdir -p ./build/
    curl -C - https://dl-cdn.alpinelinux.org/alpine/v3.22/releases/x86_64/alpine-virt-3.22.1-x86_64.iso -o build/alpine.iso
    python3 scripts/scaffold-star.py {{nodes}}

alias ds := docs-serve
docs-serve:
    mkdocs serve

alias db := docs-build
docs-build:
    mkdir -p ./build/
    mkdocs build -d build/docs

alias t := test
test: build-tests
    ./build/tests

alias bt := build-tests
build-tests:
    mkdir -p build
    clang++ -g -Iinclude tests/hello_world_test.cpp -o ./build/tests -std=c++2b -lgtest -lgtest_main -lpthread
