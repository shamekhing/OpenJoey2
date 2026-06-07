#pragma once
#include "Type.hpp"
#include "card/Card.hpp"
#include <optional>
#include <string>
#include <vector>

namespace openjoey {

// ─── CardFilter ──────────────────────────────────────────────────────────────

struct CardFilter {
  std::optional<etypes::card> type;
  std::optional<std::string> nameContains;
  std::optional<int> levelMin;
  std::optional<int> levelMax;
  std::optional<int> atkMin;
  std::optional<int> atkMax;
};

// ─── ICardRepository ─────────────────────────────────────────────────────────
// Abstract CRUD interface for card storage.
// LocalCardRepository (in-process JSON) implements this today.
// RemoteCardRepository (HTTP microservice client) will implement it later —
// the rest of the codebase never changes when that swap happens.

class ICardRepository {
public:
  virtual ~ICardRepository() = default;

  // ── Read ─────────────────────────────────────────────────────────────────

  virtual const Card *getById(uint32_t id) const = 0;
  virtual const Card *getByName(const std::string &name) const = 0;

  // Mutable lookup — used by EffectRegistry::bind() at startup only.
  // Returns nullptr in remote impls where writes go through the server.
  virtual Card *getMutableById(uint32_t /*id*/) { return nullptr; }

  // Returns pointers into the repository's owned storage — valid until
  // the next mutating call.
  virtual std::vector<const Card *> search(const CardFilter &f) const = 0;

  // All cards in load order.
  virtual const std::vector<Card> &all() const = 0;

  // ── Write ────────────────────────────────────────────────────────────────

  // Returns false if a card with the same id already exists.
  virtual bool add(Card card) = 0;

  // Replaces the card with the given id. Returns false if not found.
  virtual bool update(uint32_t id, Card card) = 0;

  // Removes the card with the given id. Returns false if not found.
  virtual bool remove(uint32_t id) = 0;

  // ── Persistence ──────────────────────────────────────────────────────────
  // LocalCardRepository: rewrites cards.json to disk.
  // RemoteCardRepository (future): POST/PUT/DELETE to the card server.
  virtual bool flush() = 0;

  // Optional: load from a local file path. No-op for remote impls.
  virtual bool loadFromFile(const std::string & /*path*/) { return false; }
};

} // namespace openjoey
