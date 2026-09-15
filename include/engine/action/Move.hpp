#pragma once
// ── act/Move — THE mat primitives (one function per mat operation) ───────────
// Pure zone::Field& operations. Every card movement in the layer goes through
// one of these. Guards (Query.hpp) decide legality; these functions do the
// moving and keep the invariant: a card never sits in two zones, even
// transiently (remove-then-put with rollback on failure).
// Leave-field sweeps are intrinsic to destroy/banish/return.
#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

#include "cards/Card.hpp"
#include "engine/field/Field.hpp"

namespace openjoey::engine::action {

using cards::Card;
using zone::Field;

inline bool MoveDestroyToGY(Field &f, Card *c);  // fwd: sweeps recurse

// ── internal helpers ────────────────────────────────────────────────────────
bool moveTo(Field &f, Card *c, zone::IZone &dest);

void detachEquip(Card *equip);

bool eraseToken(Field &f, Card *c);

bool sweepOffField(Field &f, Card *c);

// ── destroy / banish / return ───────────────────────────────────────────────
bool MoveDestroyToGY(Field &f, Card *c);
bool MoveBanish(Field &f, Card *c, bool faceDown = false);
bool MoveReturnHand(Field &f, Card *c);
bool MoveReturnDeck(Field &f, Card *c);

// ── hand / deck ─────────────────────────────────────────────────────────────
int MoveDraw(Field &f, int player, int n);
int MoveMill(Field &f, int player, int n);
int MoveDiscard(Field &f, int player, int n);
bool MoveSearchToHand(Field &f, int player, const std::function<bool(const Card &)> &pred);
std::vector<Card *> MoveExcavate(Field &f, int player, int n);

// ── summon placement (the ONE placement primitive; composites guard it) ─────
bool SummonToMMZ(Field &f, Card *c, int player, bool faceDown);

// ── positions ───────────────────────────────────────────────────────────────
bool PosFlip(Field &f, Card *c);
bool PosChange(Field &f, Card *c, zone::Orientation to);

// ── equip / counters ────────────────────────────────────────────────────────
bool EquipAttach(Card *equip, Card *monster);
bool EquipDetach(Card *equip);
void PlaceCounter(Card &c, const std::string &ctr, int n = 1);
int RemoveCounter(Card &c, const std::string &ctr, int n = 1);

// ── seat / token ────────────────────────────────────────────────────────────
bool SeatSpellTrap(Field &f, Card *c);
Card *SummonTokenMat(Field &f, const std::string &name, int atk, int def, int player);

}  // namespace openjoey::engine::action
