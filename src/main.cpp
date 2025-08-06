#include "concurrency.hpp"
#include "socks5/server.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>

using namespace toad;
using namespace toad::socks5;

int main(void) {
  auto formatter = std::make_unique<spdlog::pattern_formatter>();
  formatter->add_flag<CorrelationIdFormatter>('Z');
  formatter->set_pattern("%Y-%m-%d %H:%M:%S.%e [%L] [cid=%Z] [thread %t] %v");

  auto logger = spdlog::stdout_color_mt("console");
  logger->set_formatter(std::move(formatter));
  logger->set_level(spdlog::level::trace);
  spdlog::set_default_logger(logger);

  Executor executor;
  IOContext io_context;

  Socks5Server server;
  executor.spawn(server.serve_socks5());

  io_context.event_loop();

  return 0;
}
