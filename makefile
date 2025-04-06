make:
	mkdir -p build/
	clang -g ./src/main.cpp -o ./build/toad

setup:
	ip link set dev toad up
	ip address add dev toad local 10.0.0.5
	ip route add dev toad 10.0.0.0/24

ping:
	sudo ping 10.0.0.1 -v
