from time import sleep
from lilypad import LilypadBase

import threading


def main():
    alice = LilypadBase("alice")
    boris = LilypadBase("boris")
    colin = LilypadBase("colin")

    alice.init()
    boris.init()
    colin.init()

    alice.add_friend("boris", boris.pubkey)
    alice.add_friend("colin", colin.pubkey)
    boris.add_friend("alice", alice.pubkey)
    boris.add_friend("colin", colin.pubkey)
    colin.add_friend("alice", alice.pubkey)
    colin.add_friend("boris", boris.pubkey)

    alice.add_friend_address("boris", "127.0.0.1")
    boris.add_friend_address("alice", "127.0.0.1")

    threading.Thread(target=alice.run).start()
    threading.Thread(target=boris.run).start()
    threading.Thread(target=colin.run).start()

    sleep(36000)


if __name__ == "__main__":
    main()
