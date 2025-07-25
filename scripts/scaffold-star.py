from subprocess import Popen, PIPE, STDOUT, run
from time import sleep
from sys import argv
import getpass

ALPINE_ISO = "build/alpine.iso"
SSH_BASE_PORT = 5567
USER = getpass.getuser()

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

run(["sudo", "ip", "link", "add", f"br{0}", "type", "bridge"], check=True)
run(["sudo", "ip", "addr", "add", "192.168.100.1/24", "dev", f"br{0}"], check=True)
run(["sudo", "ip", "link", "set", f"br{0}", "up"], check=True)

run(["sudo", "dnsmasq",
  f"--interface=br{0}",
  "--bind-interfaces",
  "--except-interface=lo",
  "--dhcp-range=192.168.100.50,192.168.100.200,12h",
  "--dhcp-option=3,192.168.100.1",
  "--dhcp-option=6,192.168.100.1",
  "--no-resolv",
  "--log-queries"], check=True)


for i in range(1, NODES+1):
	run(["sudo", "ip", "tuntap", "add", "mode", "tap", "user", f"{USER}", "name", f"tap{i}"], check=True)
	run(["sudo", "ip", "link", "set", f"tap{i}", "master", f"br{0}"], check=True)
	run(["sudo", "ip", "link", "set", f"tap{i}", "up"], check=True)

for i in range(1, NODES+1):
	dom = Popen([
		"qemu-system-x86_64",
		"-m", "256M",
		"-cdrom", f"{ALPINE_ISO}",
		"-boot", "order=d",
		"-nographic",
		"-fsdev", "local,id=fsdev0,path=build,security_model=none", 
		"-device", "virtio-9p-pci,fsdev=fsdev0,mount_tag=host", 
		"-netdev", f"tap,id=net0,ifname=tap{i},script=no,downscript=no",
		"-device", f"virtio-net-pci,netdev=net0,mac=52:54:00:00:00:{13 + i:02x}",
		"-enable-kvm"
		], stdin=PIPE, stderr=PIPE, text=True, universal_newlines=True,
			bufsize=1
	)

	sleep(10)
	dom.stdin.write("root\n")
	sleep(1)
	dom.stdin.write(COMMAND)
	dom.stdin.close()
