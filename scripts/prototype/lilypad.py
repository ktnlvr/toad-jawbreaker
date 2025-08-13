import socket
from cryptography.hazmat.primitives.asymmetric.x25519 import X25519PrivateKey
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.ciphers.aead import ChaCha20Poly1305
from msgpack import dumps as pack

import os, base64, time
import asyncio
from dataclasses import dataclass, field

from handshake import create_initiation


@dataclass
class FriendRecord:
    name: str
    expected_pubkey: bytes
    ephemeral_pubkey: bytes = None
    ephemeral_privkey: bytes = None
    addresses: list[str] = field(default_factory=list)


class LilypadBase:
    def __init__(self, name):
        self.name = name
        self.port = 0
        self.server = None

        self.static_private_key = X25519PrivateKey.generate()
        self.static_public_key = self.static_private_key.public_key()

        self.friends: dict[str, FriendRecord] = {}
        self.whitelisted_ips = []

    def handler(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
        peername = writer.get_extra_info("peername")
        if not peername:
            return

        ip, port = peername[:2]
        print(f"{self.name}: incoming connection from {ip}:{port}")
        if ip not in self.whitelisted_ips:
            print(f"{self.name}: Rejecting {ip}:{port} as coming from an untrusted ip")
            return

    def add_friend(self, name: str, pubkey: bytes):
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

    def initiate_handshake(self, name, host, port):
        print(f"{self.name}: Trying to handshake {name} at {host}:{port}")
        friend = self.friends[name]
        msg, ephemeral_privkey = create_initiation(
            self.name, self.static_private_key, friend.expected_pubkey
        )
        friend.ephemeral_privkey = ephemeral_privkey
        message = pack(msg)

        self.datagram_send(message, host, port)
        print(f"{self.name}: initiation sent")

    def datagram_send(self, msg: bytes, host: str, port: int):
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.sendto(msg, (host, port))

    def run(self):
        while True:
            data, addr = self.server.recvfrom(65535)
            ip, _ = addr
            if ip not in self.whitelisted_ips:
                print(f"{self.name}: dropped packet from {addr}")
                continue
            print(f"{self.name}: received {data}")
