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
inline bool moveTo(Field &f, Card *c, zone::IZone &dest) {
    if (!c) return false;
    auto [z, p] = f.findCard(c);
    if (!z) return false;
    Card *r = z->remove(c);
    if (!r) return false;
    if (!dest.put(r)) {
        z->put(r);
        return false;
    }
    return true;
}

inline void detachEquip(Card *equip) {
    if (!equip || !equip->state.equipTarget) return;
    Card *host = equip->state.equipTarget;
    auto &eq = host->state.equippedCards;
    eq.erase(std::remove(eq.begin(), eq.end(), equip), eq.end());
    host->state.atkMod -= equip->state.bonusAtk;
    host->state.defMod -= equip->state.bonusDef;
    equip->state.equipTarget = nullptr;
}

inline bool eraseToken(Field &f, Card *c) {
    if (!c || !c->state.isToken) return false;
    if (auto [z, p] = f.findCard(c); z) z->remove(c);
    auto &tk = f.tokens;
    for (auto it = tk.begin(); it != tk.end(); ++it)
        if (it->get() == c) {
            tk.erase(it);
            return true;
        }
    return false;
}

inline bool sweepOffField(Field &f, Card *c) {
    if (!c) return false;
    detachEquip(c);
    while (!c->state.equippedCards.empty())
        if (Card *e = c->state.equippedCards.back()) MoveDestroyToGY(f, e);
        else c->state.equippedCards.pop_back();
    return eraseToken(f, c);
}

// ── destroy / banish / return ───────────────────────────────────────────────
inline bool MoveDestroyToGY(Field &f, Card *c) {
    if (!c) return false;
    if (sweepOffField(f, c)) return true;  // token erased
    int gy = c->state.controller >= 0 ? c->state.controller : 0;
    return moveTo(f, c, f.graveyardZones[gy]);
}
inline bool MoveBanish(Field &f, Card *c, bool faceDown = false) {
    if (!c) return false;
    (void)faceDown;  // ZoneStack models order only; per-card visibility not representable
    sweepOffField(f, c);
    if (eraseToken(f, c)) return true;
    int owner = c->state.owner >= 0 ? c->state.owner : 0;
    auto [z, p] = f.findCard(c);
    if (!z || !z->remove(c)) return false;
    if (!f.banishedZones[owner].put(c)) return false;
    // faceDown banish: ZoneStack models order only, per-card visibility is not
    // representable on a stack — the parameter is kept for callers.
    return true;
}
inline bool MoveReturnHand(Field &f, Card *c) {
    if (!c) return false;
    sweepOffField(f, c);
    if (eraseToken(f, c)) return true;
    int owner = c->state.owner >= 0 ? c->state.owner : 0;
    return moveTo(f, c, f.handZones[owner]);
}
inline bool MoveReturnDeck(Field &f, Card *c) {
    if (!c) return false;
    sweepOffField(f, c);
    if (eraseToken(f, c)) return true;
    int owner = c->state.owner >= 0 ? c->state.owner : 0;
    return moveTo(f, c, f.deckZones[owner]);
}

// ── hand / deck ─────────────────────────────────────────────────────────────
inline int MoveDraw(Field &f, int player, int n) {
    int drawn = 0;
    for (int i = 0; i < n; ++i) {
        if (f.deckZones[player].isEmpty()) break;
        Card *c = f.deckZones[player].remove(nullptr);  // top
        if (!c) break;
        if (!f.handZones[player].put(c)) {
            f.deckZones[player].put(c);
            break;
        }
        ++drawn;
    }
    return drawn;
}
inline int MoveMill(Field &f, int player, int n) {
    int milled = 0;
    for (int i = 0; i < n; ++i) {
        if (f.deckZones[player].isEmpty()) break;
        if (MoveDestroyToGY(f, f.deckZones[player].peek(-1))) ++milled;
    }
    return milled;
}
inline int MoveDiscard(Field &f, int player, int n) {
    int discarded = 0;
    for (int i = 0; i < n; ++i) {
        if (f.handZones[player].isEmpty()) break;
        if (MoveDestroyToGY(f, f.handZones[player].peek(-1))) ++discarded;
    }
    return discarded;
}
inline bool MoveSearchToHand(Field &f, int player, const std::function<bool(const Card &)> &pred) {
    auto &deck = f.deckZones[player];
    for (int i = 0; i < deck.count(); ++i) {
        Card *c = deck.peek(i);
        if (c && pred(*c)) return moveTo(f, c, f.handZones[player]);
    }
    return false;
}
inline std::vector<Card *> MoveExcavate(Field &f, int player, int n) {
    // Excavate = reveal the top N cards without moving them; what happens to
    // them afterwards is the realizing card's business (resolver's job).
    std::vector<Card *> out;
    auto &deck = f.deckZones[player];
    const int total = deck.count();
    for (int i = 0; i < n && i < total; ++i)
        if (Card *c = deck.peek(total - 1 - i)) out.push_back(c);  // top first
    return out;
}

// ── summon placement (the ONE placement primitive; composites guard it) ─────
inline bool SummonToMMZ(Field &f, Card *c, int player, bool faceDown) {
    if (!c) return false;
    int slot = f.monsterZones[player].firstEmpty();
    if (slot < 0) return false;
    auto [z, p] = f.findCard(c);
    if (!z || !z->remove(c)) return false;
    auto &mz = f.monsterZones[player][slot];
    if (!mz.put(c)) {
        z->put(c);
        return false;
    }
    mz.changeOrientation(faceDown ? zone::Orientation::Horizontal : zone::Orientation::Vertical);
    mz.changeVisibility(faceDown ? zone::Visibility::Limited : zone::Visibility::Visible);
    return true;
}

// ── positions ───────────────────────────────────────────────────────────────
inline bool PosFlip(Field &f, Card *c) {
    if (!c) return false;
    return f.monsterZoneOf(c) ? f.monsterZoneOf(c)->flip() : false;
}
inline bool PosChange(Field &f, Card *c, zone::Orientation to) {
    if (!c) return false;
    auto *mz = f.monsterZoneOf(c);
    return mz ? mz->changeOrientation(to) : false;
}

// ── equip / counters ────────────────────────────────────────────────────────
inline bool EquipAttach(Card *equip, Card *monster) {
    if (!equip || !monster || equip == monster) return false;
    auto &eq = monster->state.equippedCards;
    if (std::find(eq.begin(), eq.end(), equip) != eq.end()) return false;
    eq.push_back(equip);
    equip->state.equipTarget = monster;
    monster->state.atkMod += equip->state.bonusAtk;
    monster->state.defMod += equip->state.bonusDef;
    return true;
}
inline bool EquipDetach(Card *equip) {
    if (!equip || !equip->state.equipTarget) return false;
    Card *host = equip->state.equipTarget;
    auto &eq = host->state.equippedCards;
    eq.erase(std::remove(eq.begin(), eq.end(), equip), eq.end());
    host->state.atkMod -= equip->state.bonusAtk;
    host->state.defMod -= equip->state.bonusDef;
    equip->state.equipTarget = nullptr;
    return true;
}
inline void PlaceCounter(Card &c, const std::string &ctr, int n = 1) { c.state.counters[ctr] += n; }
inline int RemoveCounter(Card &c, const std::string &ctr, int n = 1) {
    auto it = c.state.counters.find(ctr);
    if (it == c.state.counters.end()) return 0;
    int rm = std::min(n, it->second);
    it->second -= rm;
    if (!it->second) c.state.counters.erase(it);
    return rm;
}

// ── seat / token ────────────────────────────────────────────────────────────
inline bool SeatSpellTrap(Field &f, Card *c) {
    if (!c) return false;
    auto [z, p] = f.findCard(c);
    if (!z || z->type() != zone::ZoneType::Hand) return false;
    int slot = f.spellTrapZones[c->state.controller].firstEmpty();
    if (slot < 0) return false;
    if (!z->remove(c)) return false;
    auto &stz = f.spellTrapZones[c->state.controller][slot];
    if (!stz.put(c)) {
        z->put(c);
        return false;
    }
    stz.changeVisibility(zone::Visibility::Limited);
    c->state.setThisTurn = true;  // p.31: Traps set this turn can't activate yet
    return true;
}
inline Card *SummonTokenMat(Field &f, const std::string &name, int atk, int def, int player) {
    int slot = f.monsterZones[player].firstEmpty();
    if (slot < 0) return nullptr;
    auto tok = std::make_unique<Card>();
    tok->name = name;
    tok->atk = atk;
    tok->def = def;
    tok->attributes.push_back(cards::Attribute::Monster);
    tok->state.owner = tok->state.controller = player;
    tok->state.isToken = true;
    Card *raw = tok.get();
    auto &mz = f.monsterZones[player][slot];
    if (!mz.put(raw)) return nullptr;
    mz.changeOrientation(zone::Orientation::Vertical);
    mz.changeVisibility(zone::Visibility::Visible);
    f.tokens.push_back(std::move(tok));
    return raw;
}

}  // namespace openjoey::engine::action
