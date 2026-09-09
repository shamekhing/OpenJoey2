#pragma once
// ── act/moves — THE mat primitives (one function per mat operation) ─────────
// Pure zone::Field& operations. Every card movement goes through one of these.
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

namespace detail {  // UI-facing relocation (resolved spells/traps -> Graveyard)
inline bool moveCard(Field &f, Card *c, zone::IZone &dest) { return moveTo(f, c, dest); }
}  // namespace detail

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
    sweepOffField(f, c);
    if (eraseToken(f, c)) return true;
    int owner = c->state.owner >= 0 ? c->state.owner : 0;
    auto [z, p] = f.findCard(c);
    if (!z || !z->remove(c)) return false;
    if (faceDown) f.banishedZones[owner].putFaceDown(c);
    else f.banishedZones[owner].put(c);
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

// ── hand / deck / materials ─────────────────────────────────────────────────
inline int MoveDraw(Field &f, int player, int n = 1) {
    int done = 0;
    for (int i = 0; i < n; ++i) {
        if (f.deckZones[player].isEmpty()) break;
        if (f.deckZones[player].moveTo(f.handZones[player])) ++done;
    }
    return done;
}
inline int MoveMillToGY(Field &f, int player, int n = 1) {
    int done = 0;
    for (int i = 0; i < n; ++i) {
        if (f.deckZones[player].isEmpty()) break;
        if (f.deckZones[player].moveTo(f.graveyardZones[player])) ++done;
        else break;
    }
    return done;
}
inline int MoveDiscardToGY(Field &f, int player, int n = 1) {
    int done = 0;
    for (int i = 0; i < n; ++i)
        if (f.handZones[player].moveTo(f.graveyardZones[player])) ++done;
    return done;
}
inline int MoveMaterialsToGY(Field &f, const std::vector<Card *> &mats) {
    int n = 0;
    for (Card *m : mats)
        if (MoveDestroyToGY(f, m)) ++n;
    return n;
}
inline int MoveSearchToHand(Field &f, int player, const std::function<bool(const Card &)> &pred) {
    for (int i = 0; i < f.deckZones[player].count(); ++i)
        if (Card *c = f.deckZones[player].peek(i))
            if (pred(*c)) return moveTo(f, c, f.handZones[player]) ? 1 : 0;
    return 0;
}
inline std::vector<Card *> MoveExcavate(Field &f, int player, int n) {
    std::vector<Card *> rev;
    auto &dk = f.deckZones[player];
    for (int i = 0; i < n && i < dk.count(); ++i)
        if (Card *c = dk.peek(-(i + 1))) rev.push_back(c);
    return rev;
}
inline int MoveDestroyMass(Field &f, openjoey::TargetScope scope, int me) {
    int n = 0;
    auto row = [&](int p) {
        for (auto &mz : f.monsterZones[p])
            if (Card *c = mz.peek()) {
                MoveDestroyToGY(f, c);
                ++n;
            }
    };
    auto st = [&] {
        for (int p = 0; p < 2; ++p)
            for (auto &z : f.spellTrapZones[p])
                if (Card *c = z.peek()) {
                    MoveDestroyToGY(f, c);
                    ++n;
                }
    };
    switch (scope) {
        case openjoey::TargetScope::OppMonsters: row(1 - me); break;
        case openjoey::TargetScope::AllMonsters:
            row(0);
            row(1);
            break;
        case openjoey::TargetScope::AllSpellsTraps: st(); break;
        case openjoey::TargetScope::OppAttackPos:
            for (auto &mz : f.monsterZones[1 - me])
                if (Card *c = mz.peek())
                    if (mz.position() == zone::Orientation::Vertical) {
                        MoveDestroyToGY(f, c);
                        ++n;
                    }
            break;
        default: break;
    }
    return n;
}

// ── placement ───────────────────────────────────────────────────────────────
// Summon pose: p.24 — Normal Summon is ATK-only / Set is face-down DEF;
// p.25 — Special Summons may choose face-up ATK, face-up DEF, or face-down DEF.
enum class SummonPose { Atk, DefUp, DefDown };

inline bool SummonToMMZ(Field &f, Card *c, int player, SummonPose pose) {
    if (!c) return false;
    auto [z, p] = f.findCard(c);
    if (!z || !z->remove(c)) return false;
    int slot = f.firstEmptyMonsterZone(player);
    if (slot < 0) {
        z->put(c);
        return false;
    }
    auto &mz = f.monsterZones[player][slot];
    if (!mz.put(c)) {
        z->put(c);
        return false;
    }
    mz.changeOrientation(pose == SummonPose::Atk ? zone::Orientation::Vertical : zone::Orientation::Horizontal);
    mz.changeVisibility(pose == SummonPose::DefDown ? zone::Visibility::Limited : zone::Visibility::Visible);
    return true;
}
// bool overloads kept for the classic summon paths (ATK or face-down DEF).
inline bool SummonToMMZ(Field &f, Card *c, int player, bool faceDown) { return SummonToMMZ(f, c, player, faceDown ? SummonPose::DefDown : SummonPose::Atk); }
inline bool SummonFromZone(Field &f, Card *c, int player, bool toEMZ, SummonPose pose) {
    if (!c) return false;
    auto [z, p] = f.findCard(c);
    if (!z || !z->remove(c)) return false;
    if (toEMZ) {
        int zi = f.firstEmptyExtraMonsterZone();
        if (zi < 0) {
            z->put(c);
            return false;
        }
        auto &emz = f.extraMonsterZones[zi];
        if (!emz.put(c)) {
            z->put(c);
            return false;
        }
        emz.changeOrientation(zone::Orientation::Vertical);
        emz.changeVisibility(zone::Visibility::Visible);
    } else {
        int slot = f.firstEmptyMonsterZone(player);
        if (slot < 0) {
            z->put(c);
            return false;
        }
        auto &mz = f.monsterZones[player][slot];
        if (!mz.put(c)) {
            z->put(c);
            return false;
        }
        mz.changeOrientation(pose == SummonPose::Atk ? zone::Orientation::Vertical : zone::Orientation::Horizontal);
        mz.changeVisibility(pose == SummonPose::DefDown ? zone::Visibility::Limited : zone::Visibility::Visible);
    }
    return true;
}
// bool overload kept for classic paths (ATK or face-down DEF).
inline bool SummonFromZone(Field &f, Card *c, int player, bool toEMZ, bool faceDown = false) { return SummonFromZone(f, c, player, toEMZ, faceDown ? SummonPose::DefDown : SummonPose::Atk); }
inline bool PlaceFusion(Field &f, Card *extra) {
    if (!extra) return false;
    auto [z, p] = f.findCard(extra);
    if (!z || z->type() != zone::ZoneType::ExtraDeck) return false;
    int zi = f.firstEmptyExtraMonsterZone();
    if (zi < 0) return false;
    if (!z->remove(extra)) return false;
    auto &emz = f.extraMonsterZones[zi];
    if (!emz.put(extra)) {
        z->put(extra);
        return false;
    }
    emz.changeOrientation(zone::Orientation::Vertical);
    emz.changeVisibility(zone::Visibility::Visible);
    return true;
}

// ── positions ───────────────────────────────────────────────────────────────
inline bool PosFlip(Field &f, Card *c) {
    if (!c) return false;
    auto [z, p] = f.findCard(c);
    if (z)
        if (auto *mz = dynamic_cast<zone::Zone_Monster *>(z)) return mz->flip();
    return false;
}
inline bool PosChange(Field &f, Card *c, zone::Orientation to) {
    if (!c) return false;
    auto [z, p] = f.findCard(c);
    if (z)
        if (auto *mz = dynamic_cast<zone::Zone_Monster *>(z)) return mz->changeOrientation(to);
    return false;
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
    int slot = f.firstEmptySpellTrapZone(c->state.controller);
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
    int slot = f.firstEmptyMonsterZone(player);
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
