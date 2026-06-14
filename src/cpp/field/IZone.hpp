#pragma once
#include "Type.hpp"
#include "card/Card.hpp"
#include <algorithm>
#include <functional>
#include <random>
#include <vector>

namespace openjoey::zone {
/**
 * Base interface for all zones.
 *
 * A zone never owns cards; it stores raw pointers to Card values owned by
 * DuelCore. Failed move operations roll cards back to their source zone.
 */
class IZone {
public:
  virtual ~IZone() = default;
  virtual etypes::zone type() const = 0;
  virtual bool isEmpty() const = 0;
  virtual int count() const = 0;
  virtual bool put(Card *c) = 0;
  virtual bool contains(const Card *c) const = 0;
  virtual void reset() {}
  virtual Card *remove(Card *c = nullptr) = 0; // nullptr → remove top.
  virtual Card *peek(int = -1) const { return nullptr; }

  // Move the top/only card to dest. Rolls back if dest.put fails.
  bool moveTo(IZone &dest) {
    Card *c = remove(nullptr);
    return !c ? false : (!dest.put(c) ? (put(c), false) : true);
  }

  bool is(etypes::orientation ori) const { return orientation_ == ori; }
  bool is(etypes::visibility vis) const { return visibility_ == vis; }

  bool isOwner(int player) const { return owner_ == player; }
  bool canView(int player) const {
    return is(etypes::visibility::Visible) ||
           (!is(etypes::visibility::Restricted) && isOwner(player));
  }

  void setOwner(int player) { owner_ = player; }
  int owner() const { return owner_; }

protected:
  bool isStack_ = false; // for type checking in FieldGrid
  int owner_ = -1;       // 0 or 1 for player, or -1 for shared zones like EMZ
  etypes::orientation orientation_ = etypes::orientation::Vertical;
  etypes::visibility visibility_ = etypes::visibility::Visible;
};

} // namespace openjoey::zone
