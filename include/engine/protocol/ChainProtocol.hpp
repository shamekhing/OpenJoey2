#pragma once
// ── ChainProtocol — the chain resolution walk state ──────────────────────────
#include <cstdint>

namespace openjoey::engine::protocol {

enum class ChainStep : uint8_t {
  Idle,       // no chain open
  Building,   // links being added (response window open)
  Responding, // responder deciding: chain or pass
  Resolving,  // reverse order, one link at a time
  Resolved,   // chain fully resolved
};

} // namespace openjoey::engine::protocol
