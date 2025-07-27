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

      auto data = co_await io.submit_read_some(client, 2);
      if (!data)
        continue;
      auto buffer = std::move(data.value());

      if (buffer.size < 2)
        continue;

      u8 version = buffer[0];
      u8 method_count = buffer[1];

      if (version != 5) {
        spdlog::warn(
            "Attempt to connect with version {}, but only 5 is supported",
            version);
        continue;
      }

      spdlog::info("Connection using V{} with {} authentication methods",
                   version, method_count);

      auto data_methods = co_await io.submit_read_some(client, method_count);
      if (!data_methods)
        continue;
      auto methods = std::move(data_methods.value());

      bool is_ok = false;
      for (u8 i = 0; i < method_count; i++)
        // Allow no authentication
        if (methods[i] == 0x00)
          is_ok = true;

      if (!is_ok) {
        spdlog::warn("Authentication failed. Pity.");
        continue;
      }

      std::array<u8, 2> response = {0x05, 0x00};
      io.submit_write_some(client, std::span(response));

      // Client accepted! Can do some real processing now.
      // the connection is trusted
      spdlog::info("Trusted connection established, can do some real work now");

      auto command_data = co_await io.submit_read_some(client, 4);
      if (!command_data)
        continue;
      auto command = command_data.value();

      if (command[0] != 0x05) // only version 5 supported
        continue;
      if (command[1] != 0x01) // only "CONNECT" is supported
        continue;
      auto atype = command[3];

      std::variant<IPv4, std::string> address;
      switch (atype) {
      case 0x01: { // IPv4
        spdlog::info("Connecting via IPv4");
        auto buffer_data = co_await io.submit_read_some(client, 4);
        if (!buffer_data)
          continue;
        auto b = buffer_data.value();
        auto ip = IPv4({b[0], b[1], b[2], b[3]});
        spdlog::info("Connecting via IPv4 to {}", ip);
        address = ip;
      } break;
      case 0x03: { // Domain Name
        auto domain_name_length_data = co_await io.submit_read_some(client, 1);

        if (!domain_name_length_data)
          continue;

        // TODO(Artur): add better reading methods I beg
        auto domain_name_length = domain_name_length_data.value()[0];

        auto domain_name_data =
            co_await io.submit_read_some(client, domain_name_length);
        if (!domain_name_data)
          continue;
        auto domain_name = domain_name_data.value();

        auto domain =
            std::string((const char *)domain_name.data(), domain_name_length);

        address = domain;
      } break;
      case 0x04: // IPv6
        spdlog::error("IPv6 not supported, sorry");
        continue;
      default:
        spdlog::error("Unknown address type 0x{:02X}", atype);
        continue;
      }

      auto port_data = co_await io.submit_read_some(client, 2);
      if (!port_data)
        continue;

      u16 port;
      ByteIStream(port_data.value()).read_u16(&port);

      spdlog::info("Connecting at port {}", port);

      auto &ipv4 = std::get<IPv4>(address);
      auto server_response = co_await io.submit_connect_ipv4(ipv4, port);
      if (!server_response) {
        spdlog::error("Connecting to {} failed", ipv4);
        continue;
      }

      spdlog::info("Reception logic done, terminating connection. Real handler "
                   "would take off from here.");
    }
  }
};

} // namespace toad::socks5