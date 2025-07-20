# Toad Jawbreaker

## Purpose

Toad is a distributed networking service for dense private networks where connectivity is unstable and/or being actively compromised. It attempts to mitigate the issues of failing connectivity by utilizing mesh routing.

## Audience

This project is intended for relatively-small scale networking with high bandwidth applications in a high-trust environment. 

## Context

There are a lot of similar projects:

- VPNs like [OpenVPN](https://openvpn.net/) and [Wireguard](https://www.wireguard.com/)
- Private networking solutions like [Innernet](https://github.com/tonarino/innernet) and [Tailscale](https://tailscale.com/).

Toad is trying to hit a different niche. It is for cases when the outer network is being actively disturbed.

## Goals

- **Resilient**. Detecting and routing around network failures.
- **High Throughput / Low Latency**. Leveraging efficient transport protocols and optimized routing for bandwidth-intensive applications.
- **Decentralised**. Operating without a centralised authority.
- **Ease of Deployment**. Trivially adding new nodes.
- **Peer Connections**. Facilitating direct inter-peer interactions within the network without direct P2P connections.

## Non-Goals

- **Total Zero-trust**. The network is unlikely to recover if large portions of it are compromised.
- **Massive Scale Routing**. Not optimized for a thousand of nodes, best for networks of a few hundred active participants.
- **Commercial Ties**. No builtin management utilities or SLA guarantees.

## Tech Stack

## Security Considerations

## Examples

## Code Style

Heavily inspired by [TigerStyle](https://github.com/tigerbeetle/tigerbeetle/blob/main/docs/TIGER_STYLE.md) and [NASA's Power of Ten](https://spinroot.com/gerard/pdf/P10.pdf). Adhering to this should be just fine.
