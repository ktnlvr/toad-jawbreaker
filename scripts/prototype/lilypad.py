import socket
from cryptography.hazmat.primitives.asymmetric.x25519 import X25519PrivateKey
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.ciphers.aead import ChaCha20Poly1305
from msgpack import dumps as pack, loads as unpack

import os, base64, time
import asyncio
from dataclasses import dataclass, field

from handshake import create_initiation, finish_handshake, respond_to_initiation


@dataclass
class FriendRecord:
    name: str
    expected_pubkey: bytes
    ephemeral_privkey: X25519PrivateKey

    send_key: bytes = None
    recv_key: bytes = None

    addresses: list[str] = field(default_factory=list)
    handshake_established: bool = field(default=False)


class LilypadBase:
    def __init__(self, name):
        self.name = name
        self.port = 0
        self.server = None

        self.static_private_key = X25519PrivateKey.generate()
        self.static_public_key = self.static_private_key.public_key()

        self.friends: dict[str, FriendRecord] = {}
        self.whitelisted_ips = []

        self.handlers = {}

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
        print(f"{self.name}: trying to handshake {name} at {host}:{port}")
        friend = self.friends[name]
        initiation, ephemeral_privkey = create_initiation(
            self.name, self.static_private_key, friend.expected_pubkey
        )
        friend.ephemeral_privkey = ephemeral_privkey
        self.datagram_send({"kind": "handshake?", "payload": initiation}, host, port)
        print(f"{self.name}: initiation sent")

    def respond_to_handshake(
        self, initiator_msg: dict, name: str, host: str, port: int
    ):
        friend = self.friends[name]

        response, send_key, recv_key, ephemeral_private_key = respond_to_initiation(
            self.name, self.static_private_key, initiator_msg
        )

        friend.recv_key = recv_key
        friend.send_key = send_key
        friend.ephemeral_privkey = ephemeral_private_key

        self.datagram_send({"kind": "handshake!", "payload": response}, host, port)

    def finalize_handshake(self, name: str, host: str, port: str):
        friend = self.friends[name]
        finish_handshake(
            self.static_private_key,
            friend.ephemeral_privkey,
            friend.expected_pubkey,
        )

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
