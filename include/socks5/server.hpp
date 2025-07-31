#pragma once

#include "../bytes.hpp"
#include "../concurrency.hpp"

namespace toad::socks5 {

struct Socks5Server {
  Task<void> serve_socks5() {
    IOContext &io = this_io_context();

    auto listener = io.new_listener(1080);
    spdlog::info("Expecting SOCKS5 connections on 0.0.0.0:1080");

    while (true) {
      auto client = co_await io.submit_accept_ipv4(listener);
      spdlog::info("Got client sockfd={}", client._sockfd);
      spawn(handle_client_handshake(std::move(client)));
    }
  }

  Task<void> handle_client_handshake(Socket client) {
    IOContext &io = this_io_context();

    std::vector<u8> buffer;
    ByteIStream<std::vector<u8>> istream(buffer);

    auto read = co_await io.submit_read_some(client, buffer, 2);
    if (read < 2)
      co_return;

    u8 version, nmethods;
    istream.read_u8(&version).read_u8(&nmethods);

    if (version != 5) {
      spdlog::warn(
          "Attempt to connect with version {}, but only 5 is supported",
          version);
      co_return;
    }

    spdlog::info("Connection using V{} with {} authentication methods", version,
                 nmethods);

    auto methods = co_await io.submit_read_some(client, buffer, nmethods);
    if (methods < nmethods) {
      spdlog::error("Not enough Auth methods provided");
      co_return;
    }

    bool is_ok = false;
    for (u8 i = 0; istream.remaining(); i++) {
      u8 byte;
      istream.read_u8(&byte);
      if (byte == 0x00)
        is_ok = true;
    }

    if (!is_ok) {
      spdlog::warn("Authentication failed. Pity.");
      co_return;
    }

    std::array<u8, 2> response = {0x05, 0x00};
    io.submit_write_some(client, std::span(response));

    // Client accepted! Can do some real processing now.
    // the connection is trusted
    spdlog::info("Trusted connection established, can do some real work now");

    read = co_await io.submit_read_some(client, buffer, 4);
    if (read < 4)
      co_return;

    u8 command, reserved, atype;
    istream.read_u8(&version).read_u8(&command).read_u8(&reserved).read_u8(
        &atype);

    if (version != 0x05) // only version 5 supported
      co_return;
    if (command != 0x01) // only "CONNECT" is supported
      co_return;

    std::variant<IPv4, std::string> address;
    switch (atype) {
    case 0x01: { // IPv4
      auto read = co_await io.submit_read_some(client, buffer, 4);
      if (read < 4)
        co_return;
      IPv4 addr;
      istream.read_array(&addr);
      spdlog::critical("{}", addr);
      address = addr;
    } break;
    case 0x03: { // Domain Name
      spdlog::info("Domain Names not supported yet :c");
    } break;
    case 0x04: // IPv6
      spdlog::error("IPv6 not supported, sorry");
      co_return;
    default:
      spdlog::error("Unknown address type 0x{:02X}", atype);
      co_return;
    }

    co_await io.submit_read_some(client, buffer, 2);

    u16 port;
    istream.read_u16(&port);

    spdlog::debug("{}", buffer);

    auto ipv4 = std::get<IPv4>(address);
    auto maybe_remote_connection = co_await handle_connection(ipv4, port);
    if (!maybe_remote_connection) {
      spdlog::error("Failed to establish a connection with the remote {}:{}",
                    ipv4, port);
      co_return;
    }

    auto remote = std::move(maybe_remote_connection.value());

    spawn(client_to_remote(client, remote));
    spawn(remote_to_client(client, remote));

    spdlog::info("Now transmitting!");
    spdlog::info("Transmission over!");
  }

  Future<std::optional<Socket>> handle_connection(const IPv4 &target,
                                                  u16 port) {
    IOContext &io = this_io_context();

    spdlog::info("Connecting to {}:{}", target, port);

    return io.submit_connect_ipv4(target, port);
  }

  Task<void> client_to_remote(const Socket &client, const Socket &remote) {
    spdlog::info("Client -> Remote done");
    co_return;
  }

  Task<void> remote_to_client(const Socket &client, const Socket &remote) {
    spdlog::info("Remote -> Client done");
    co_return;
  }
};

} // namespace toad::socks5