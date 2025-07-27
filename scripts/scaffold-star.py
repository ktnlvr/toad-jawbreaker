from subprocess import Popen, DEVNULL, PIPE, STDOUT, run
from time import sleep
from sys import argv, exit
import signal
from getpass import getuser

ALPINE_ISO = "build/alpine.iso"
VM_SUBNET = "192.168.100.1/24"
USER = getuser()
DNSMASQ_PID_FILE = "/tmp/dnsmasq.pid"

DNSMASQ_PARAMS = ["sudo", "dnsmasq",
	f"--interface=br{0}",
	"--bind-interfaces",
	"--except-interface=lo",
	"--dhcp-range=192.168.100.50,192.168.100.200,12h",
	"--dhcp-option=3,192.168.100.1",
	"--dhcp-option=6,192.168.100.1",
	"--no-resolv",
	"--log-queries",
	f"--pid-file={DNSMASQ_PID_FILE}"]

VM_PARAMS = [
	"qemu-system-x86_64",
	"-m", "256M",
	"-cdrom", f"{ALPINE_ISO}",
	"-boot", "order=d",
	"-nographic",
	"-enable-kvm",
	"-fsdev", "local,id=fsdev0,path=build,security_model=none", 
	"-device", "virtio-9p-pci,fsdev=fsdev0,mount_tag=host", 
	"-netdev", "tap,id=net0,ifname=tap{i},script=no,downscript=no",
	"-device", "virtio-net-pci,netdev=net0,mac=52:54:00:00:00:{mac:02x}"
]

COMMANDS = [
	"setup-alpine -q",
	"apk add openssh",
	"echo PermitRootLogin yes >> /etc/ssh/sshd_config",
	"echo PermitEmptyPasswords yes >> /etc/ssh/sshd_config",
	"rc-service sshd start",
	"mkdir /mnt/host",
	"mount -t 9p -o trans=virtio host /mnt/host",
	"cp /mnt/host/main /main",
	"umount /mnt/host",
	#"/main"
]
COMMAND = ''.join(x + " && " for x in COMMANDS[:-1]) + COMMANDS[-1] + "\n"
NODES = int(argv[1])

processes = []
devices = []

def cleanup():
	# print(processes, devices)

	for pid in processes:
		run(['sudo', 'kill', f"{pid}"])

	for dev in devices:
		run(["sudo", "ip", "link", "delete", f"{dev}"], check=True)
	

def handler(sig, frame):
	cleanup()
	exit(0)

signal.signal(signal.SIGINT, handler)

try:
	run(["sudo", "ip", "link", "add", f"br{0}", "type", "bridge"], check=True)
	run(["sudo", "ip", "addr", "add", f"{VM_SUBNET}", "dev", f"br{0}"], check=True)
	run(["sudo", "ip", "link", "set", f"br{0}", "up"], check=True)
	devices.append(f"br{0}")

	dnsmasq = Popen(DNSMASQ_PARAMS)
	sleep(1)
	with open("/tmp/dnsmasq.pid") as f:
		dnsmasq_pid = int(f.read().strip())
		processes.append(dnsmasq_pid)

	for i in range(1, NODES+1):
		run(["sudo", "ip", "tuntap", "add", "mode", "tap", "user", f"{USER}", "name", f"tap{i}"], check=True)
		run(["sudo", "ip", "link", "set", f"tap{i}", "master", f"br{0}"], check=True)
		run(["sudo", "ip", "link", "set", f"tap{i}", "up"], check=True)
		devices += [f"tap{i}"]

	for i in range(1, NODES+1):
		data = {"i" : i, "mac" : i}
		params = [param.format(**data) for param in VM_PARAMS]
		vm = Popen(params, stdin=PIPE, stdout=DEVNULL, text=True, universal_newlines=True, bufsize=1)
		processes.append(vm.pid)

		sleep(10)
		vm.stdin.write("root\n")
		sleep(1)
		vm.stdin.write(COMMAND)
		vm.stdin.close()

	print("Ready, to stop press CTRL + C")
except:
	cleanup()
	exit(0)

signal.pause()
