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

      if (auto data = co_await io.submit_read_some(client, 2)) {
        auto buffer = std::move(data.value());
        if (buffer.size < 2)
          continue;

        u8 version = buffer.data()[0];
        u8 method_count = buffer.data()[1];

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
          if (methods.data()[i] == 0x00)
            is_ok = true;

        if (!is_ok) {
          spdlog::warn("Authentication failed. Pity.");
          continue;
        }

        std::array<u8, 2> response = {0x05, 0x00};
        io.submit_write_some(client, std::span(response));

        // Client accepted! Can do some real processing now.
        // the connection is trusted
        spdlog::info(
            "Trusted connection established, can do some real work now");
      }
    }
  }
};

} // namespace toad::socks5