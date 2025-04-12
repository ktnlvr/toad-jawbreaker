mprocs = $${HOME}/.cargo/bin/mprocs

make:
	mkdir -p build/
	clang -g ./src/main.cpp -o ./build/toad -std=c++20 -lstdc++

run: make
	sudo $(mprocs) "./build/toad" "sleep 1s; make setup-network-device;" "sleep 2s; tshark -i toad" "sleep 2s; make ping"

troubleshoot: make
	sudo $(mprocs) "sleep 1s; make setup-network-device;" "sleep 2s; tshark -i toad" "sleep 2s; make ping"

orchestrate: make
	sudo $(mprocs) "sudo lldb-server platform --server --listen *:1234" "sleep 1s; make setup-network-device;" "sleep 2s; tshark -i toad" "sleep 2s; make ping"

setup-network-device:
	ip link set dev toad up
	ip address add dev toad local 10.0.0.5
	ip route add dev toad 10.0.0.0/24
	ip addr show dev toad

ping:
	sudo ping 10.0.0.1 -v
