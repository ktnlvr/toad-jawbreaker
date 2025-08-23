# TX

The transport layer is responsible for handling the communication between trusted nodes on the network.

Any transport starts with a HANDSHAKE.

```
A <-[Pub(B)]-- B

...

A --[Pub(A)]-> B

A <-[E_BA(RandInt)]- B

A -[E_AB(RandInt)]-> B
```

When the handshake is over the transport may begin. The handshake has to be performed for every connection.

After a handshake is performed both parties exchange routing information using ROUTE packets. The routing table is a diff from the previous form. The exchange is performed until all hashes are identical.

```
+ S T Pub(C)    -- An added client C with sequence number S and two-way travel time T
- S T Pub(D)    -- A removed client D with sequence number S and two-way travel time T
```

When a node is trying to reach out to another node but can't find its address, it sends out a STARVE packet:

```
Pub(T)          -- Pubkey of the target that the node is trying to reach.
```

If there is no response from the network in a reasonable amount of time, the target is deemed unreachable.

A transport (TX) message for sender A and receiver B is as follows...

```
S_A(...)        -- Cryptographic signature of the sender
Ts              -- Timestamp of when the message was sent
H(Pub(A))       -- Hashed public key of A
H(Pub(B))       -- Hashed public key of B
len(Payload)    -- Length of the payload (u16)
E_AB(Payload)   -- Encrypted payload going from A to B
```
