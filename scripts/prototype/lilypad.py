import socket
from cryptography.hazmat.primitives.asymmetric.x25519 import (
    X25519PrivateKey,
    X25519PublicKey,
)
from msgpack import dumps as pack, loads as unpack

from dataclasses import dataclass, field


@dataclass
class FriendRecord:
    name: str
    expected_pubkey: X25519PublicKey
    addresses: list[str] = field(default_factory=list)


class LilypadBase:
    def __init__(self, name):
        self.name = name
        self.port = 0
        self.server = None

        self.static_private_key = X25519PrivateKey.generate()
        self.pubkey = self.static_private_key.public_key()

        self.friends: dict[str, FriendRecord] = {}
        self.whitelisted_ips = []

        self.handlers = {}

    def add_friend(self, name: str, pubkey: X25519PublicKey):
        self.friends[name] = FriendRecord(name=name, expected_pubkey=pubkey)

    def add_friend_address(self, name: str, address: str):
        address  # actually this should be an address mask
        self.friends[name].addresses.append(address)
        self.whitelisted_ips.append(address)

    def init(self):
        self.server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.server.bind(("0.0.0.0", 0))
        self.port = self.server.getsockname()[1]
        print(f"{self.name}: bound to port {self.port}")

    def datagram_send(self, msg: bytes | dict, host: str, port: int):
        if isinstance(msg, dict):
            msg = pack(msg)

        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.sendto(msg, (host, port))

    def run(self):
        while True:
            data, addr = self.server.recvfrom(65535)
            ip, _ = addr
            if ip not in self.whitelisted_ips:
                print(f"{self.name}: packet dropped, {addr} unknown")
                continue

            message = unpack(data)
            print(f"{self.name}: received {message}")

            if (kind := message.get("kind")) is None:
                print(f"{self.name}: packet dropped, no kind specified")
                continue

            if (handler := self.handlers.get(kind)) is None:
                print(
                    f"{self.name}: packet dropped, no handler for kind `{kind}` found"
                )
                continue

            if payload := message.get("payload"):
                handler(payload)
            else:
                handler()
