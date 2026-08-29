#pragma once
#include "card/Card.hpp"
#include "card/EffectID.hpp"
#include "zone/Zone.hpp"
#include "field/Field.hpp"
#include <string>

namespace openjoey {

// ── Effects layer ───────────────────────────────────────────────────────────
//
// Layer 3 (depends on zone/ + card/).  Embodied by the Rulebook insight:
// *most effects can be solved by moving a card from one zone to another*.
// Every function here is expressed as one or more remove()/put()/moveTo()
// calls against zones — an effect handler never rewrites a card's ATK/DEF
// directly.  This is the single region that mutates zones on behalf of a card
// effect; the player's drag-drop in field/ui/FieldGrid is separate.
//
namespace effect_detail {

// Map a ZoneType to the Location the Rulebook would call home.
inline Location locOf(zone::ZoneType t) {
  using zone::ZoneType;
  switch (t) {
  case ZoneType::Hand: return Card::Location::Hand;
  case ZoneType::Deck: return Card::Location::Deck;
  case ZoneType::ExtraDeck: return Card::Location::ExtraDeck;
  case ZoneType::Graveyard: return Card::Location::Graveyard;
  case ZoneType::Banished: return Card::Location::Banished;
  case ZoneType::SideDeck: return Card::Location::None;
  default: return Card::Location::Field; // Monster / SpellTrap / Field / EMZ
  }
}

// Sync a card's Location field to the zone that now holds it.
inline void setLoc(Card *c, zone::IZone *dest) {
  if (c)
    c->location = locOf(dest->type());
}

// Targeted single-card transfer with rollback. Unlike IZone::moveTo (which
// only lifts the *top* card), this lifts a *specific* card pointer.
inline bool moveCard(zone::Field &f, Card *c, zone::IZone &dest) {
  if (!c)
    return false;
  auto [z, p] = f.findCard(c);
  if (!z)
    return false;
  Card *r = z->remove(c); // lift the specific card
  if (!r)
    return false;
  if (!dest.put(r)) { // rollback on a full destination
    z->put(r);
    return false;
  }
  setLoc(c, &dest);
  return true;
}
} // namespace effect_detail

// ── hand / deck movements ───────────────────────────────────────────────────

// Deck -> Hand, n times. Returns cards actually drawn.
inline int Move_Draw(zone::Field &f, int player, int n = 1) {
  int done = 0;
  for (int i = 0; i < n; ++i) {
    if (f.deckZones[player].isEmpty())
      break;
    if (f.deckZones[player].moveTo(f.handZones[player])) {
            f.handZones[player].peek(-1)->location = Location::Hand;
      ++done;
    }
  }
  return done;
}

// Deck -> Graveyard ("mill"/"send to GY").
inline int Move_MillToGY(zone::Field &f, int player, int n = 1) {
  int done = 0;
  for (int i = 0; i < n; ++i)
    if (f.deckZones[player].moveTo(f.graveyardZones[player]))
      ++done;
  return done;
}

// Hand -> Graveyard (discard).
inline int Move_DiscardToGY(zone::Field &f, int player, int n = 1) {
  int done = 0;
  for (int i = 0; i < n; ++i)
    if (f.handZones[player].moveTo(f.graveyardZones[player]))
      ++done;
  return done;
}

// ── field -> graveyard ──────────────────────────────────────────────────────
// Destroy a card on the field: send it to its controller's Graveyard.
inline bool Move_DestroyToGY(zone::Field &f, Card *c) {
  if (!c)
    return false;
  auto [z, p] = f.findCard(c);
  if (!z)
    return false;
  int gyOwner = c->controller;
  if (gyOwner < 0)
    gyOwner = (p >= 0) ? p : 0;
  return effect_detail::moveCard(f, c, f.graveyardZones[gyOwner]);
}

// ── removal / return ────────────────────────────────────────────────────────
// Banish a card from anywhere. faceDown=true -> hidden banish (cost).
inline bool Move_Banish(zone::Field &f, Card *c, bool faceDown = false) {
  if (!c)
    return false;
  auto [z, p] = f.findCard(c);
  if (!z || !z->remove(c))
    return false;
  int owner = c->owner;
  if (owner < 0)
    owner = (p >= 0) ? p : 0;
  if (faceDown)
    f.banishedZones[owner].putFaceDown(c);
  else
    f.banishedZones[owner].put(c);
  effect_detail::setLoc(c, &f.banishedZones[owner]);
  return true;
}

inline bool Move_ReturnHand(zone::Field &f, Card *c) {
  if (!c)
    return false;
  auto [z, p] = f.findCard(c);
  if (!z)
    return false;
  int owner = c->owner;
  if (owner < 0)
    owner = (p >= 0) ? p : 0;
  return effect_detail::moveCard(f, c, f.handZones[owner]);
}

inline bool Move_ReturnDeck(zone::Field &f, Card *c) {
  if (!c)
    return false;
  auto [z, p] = f.findCard(c);
  if (!z)
    return false;
  int owner = c->owner;
  if (owner < 0)
    owner = (p >= 0) ? p : 0;
  return effect_detail::moveCard(f, c, f.deckZones[owner]);
}

// ── summons ─────────────────────────────────────────────────────────────────
// Hand / Deck / Extra Deck / Graveyard -> Monster zone or EMZ, with placement.
inline bool Summon_ToMMZ(zone::Field &f, Card *c, int toPlayer, bool faceDown) {
  if (!c)
    return false;
  auto [z, p] = f.findCard(c);
  if (!z || !z->remove(c))
    return false;
  int slot = f.firstEmptyMonsterZone(toPlayer);
  if (slot < 0) {
    z->put(c);
    return false;
  }
  auto &mz = f.monsterZones[toPlayer][slot];
  if (!mz.put(c)) {
    z->put(c);
    return false;
  }
  if (faceDown) {
    mz.changeOrientation(zone::Orientation::Horizontal);
    mz.changeVisibility(zone::Visibility::Restricted);
  } else {
    mz.changeOrientation(zone::Orientation::Vertical);
    mz.changeVisibility(zone::Visibility::Visible);
  }
  effect_detail::setLoc(c, &mz);
  return true;
}

inline bool Summon_Normal(zone::Field &f, Card *c, int toPlayer) {
  return Summon_ToMMZ(f, c, toPlayer, false);
}

inline bool Summon_Set(zone::Field &f, Card *c, int toPlayer) {
  return Summon_ToMMZ(f, c, toPlayer, true);
}

inline bool Summon_Special(zone::Field &f, Card *c, int toPlayer,
                           bool toEMZ = false) {
  if (!c)
    return false;
  auto [z, p] = f.findCard(c);
  if (!z || !z->remove(c))
    return false;
  zone::IZone *dest;
  if (toEMZ) {
    int zi = f.firstEmptyExtraMonsterZone();
    if (zi < 0) {
      z->put(c);
      return false;
    }
    dest = &f.extraMonsterZones[zi];
  } else {
    int slot = f.firstEmptyMonsterZone(toPlayer);
    if (slot < 0) {
      z->put(c);
      return false;
    }
    dest = &f.monsterZones[toPlayer][slot];
  }
  if (!dest->put(c)) {
    z->put(c);
    return false;
  }
  effect_detail::setLoc(c, dest);
  return true;
}

// Flip a Face-Down Defense Position monster Face-Up (its Flip effect then
// triggers — handled by the engine, not the zone layer).
inline bool Pos_Flip(zone::Field &f, Card *c) {
  if (!c)
    return false;
  auto [z, p] = f.findCard(c);
  if (z)
    if (auto *mz = dynamic_cast<zone::Zone_Monster *>(z))
            return mz->flip();
  return false;
}

} // namespace openjoey

