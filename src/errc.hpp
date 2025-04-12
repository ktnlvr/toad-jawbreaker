#pragma once

#include "defs.hpp"

namespace toad {

// The error is not recoverable, all relevant resourced
// might be in a malformed state
#define ERRC_MASK_CRITICAL 0x8000
#define ERRC_MASK_MALFORMED_INPUT 0x4000

enum struct ErrorCode : u16 {
  OK = 0,

  CODE_PATH_NOT_HANDLED = 0x01,
  NOT_ENOUGH_DATA = 0x02,

  INVALID_ARP_OPERATION = 0x45,

  SOCKET_INIT_FAILED = 0x54,
  SOCKET_SETUP_FAILED = 0x55,
  SOCKET_WRITE_FAILED = 0x56,
  SOCKET_READ_FAILED = 0x57,

  // Not enough data in the input buffer to possibly perform the operation.
  BUFFER_TOO_SMALL = 0x100 | ERRC_MASK_MALFORMED_INPUT,
  // Nullptr passed as an output parameter
  NULLPTR_OUTPUT = 0x101 | ERRC_MASK_MALFORMED_INPUT,
};

bool is_errc_malformed_input(ErrorCode errc) {
  return (u16)errc & ERRC_MASK_MALFORMED_INPUT;
}

}  // namespace toad
