from time import sleep
from lilypad import LilypadBase

import asyncio
import threading


def main():
    alice = LilypadBase("alice")
    boris = LilypadBase("boris")
    colin = LilypadBase("colin")

    alice.init()
    boris.init()
    colin.init()

    alice.add_friend("boris", boris.static_public_key.public_bytes_raw())
    alice.add_friend("colin", colin.static_public_key.public_bytes_raw())
    boris.add_friend("alice", alice.static_public_key.public_bytes_raw())
    boris.add_friend("colin", colin.static_public_key.public_bytes_raw())
    colin.add_friend("alice", alice.static_public_key.public_bytes_raw())
    colin.add_friend("boris", boris.static_public_key.public_bytes_raw())

    alice.add_friend_address("boris", "127.0.0.1")
    alice.initiate_handshake("boris", "127.0.0.1", boris.port)
    boris.add_friend_address("alice", "127.0.0.1")

    threading.Thread(target=alice.run).start()
    threading.Thread(target=boris.run).start()
    threading.Thread(target=colin.run).start()

    sleep(36000)


if __name__ == "__main__":
    main()
