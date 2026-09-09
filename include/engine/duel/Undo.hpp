#pragma once
// ── Undo (duel/Undo.hpp) — snapshot / restore for one-step (bounded) undo ───
// A Duel snapshot deep-copies only what the engine owns:
//   * zones            — copied (they hold raw Card*; external deck-owned
//                        cards keep their addresses, so those pointers copy
//                        as-is)
//   * tokens           — DEEP-copied (unique_ptr), then every zone pointer to
//                        an old token is remapped to the clone (replacePtr)
//   * CardState of the referenced external cards — snapshotted separately and
//                        re-applied on restore (equips/counters/turn flags
//                        live on the shared card objects)
// turnState and chain link args are remapped for tokens as well.
#include <map>
#include <memory>
#include <vector>

#include "engine/duel/Duel.hpp"

namespace openjoey::engine {

// Deep-clone a Duel. `cardStates` (optional) collects the CardState of every
// non-token card referenced by the field, for restore-time re-application.
inline Duel cloneDuel(const Duel &src, std::map<Card *, cards::CardState> *cardStates = nullptr) {
    Duel dst;
    dst.config = src.config;
    dst.turn = src.turn;
    dst.chain = src.chain;
    dst.lp = src.lp;
    dst.turnPlayer = src.turnPlayer;
    dst.activePlayer = src.activePlayer;
    dst.result = src.result;
    dst.winReason = src.winReason;
    dst.turnState = src.turnState;
    dst.battleStep = src.battleStep;
    dst.damageStep = src.damageStep;
    dst.lastDamageOutcome = src.lastDamageOutcome;
    dst.battleTrace = src.battleTrace;
    dst.deckBackings = src.deckBackings;

    zone::Field &f = dst.field;
    const zone::Field &sf = src.field;
    for (int p = 0; p < zone::Field::PLAYERS; ++p) {
        for (int i = 0; i < zone::Field::MONSTER_ZONES; ++i) {
            f.monsterZones[p][i] = sf.monsterZones[p][i];
            f.spellTrapZones[p][i] = sf.spellTrapZones[p][i];
        }
        f.fieldZones[p] = sf.fieldZones[p];
        f.handZones[p] = sf.handZones[p];
        f.deckZones[p] = sf.deckZones[p];
        f.extraDeckZones[p] = sf.extraDeckZones[p];
        f.graveyardZones[p] = sf.graveyardZones[p];
        f.banishedZones[p] = sf.banishedZones[p];
        f.sideDeckZones[p] = sf.sideDeckZones[p];
    }
    for (int z = 0; z < zone::Field::EMZ_COUNT; ++z) f.extraMonsterZones[z] = sf.extraMonsterZones[z];

    // Deep-copy tokens and build the old→new pointer map.
    std::map<Card *, Card *> remap;
    for (const auto &t : sf.tokens) {
        auto copy = std::make_unique<Card>(*t);
        remap[t.get()] = copy.get();
        f.tokens.push_back(std::move(copy));
    }
    f.remapPointers(remap);

    // Collect external card states (shared objects — snapshot their state).
    if (cardStates) {
        auto record = [&](const Card *c) {
            if (c && !c->state.isToken) (*cardStates)[const_cast<Card *>(c)] = c->state;
        };
        for (int p = 0; p < zone::Field::PLAYERS; ++p) {
            for (auto &z : sf.monsterZones[p]) record(z.peek());
            for (auto &z : sf.spellTrapZones[p]) record(z.peek());
            record(sf.fieldZones[p].peek());
            for (int i = 0; i < sf.handZones[p].count(); ++i) record(sf.handZones[p].peek(i));
            for (int i = 0; i < sf.deckZones[p].count(); ++i) record(sf.deckZones[p].peek(i));
            for (int i = 0; i < sf.extraDeckZones[p].count(); ++i) record(sf.extraDeckZones[p].peek(i));
            for (int i = 0; i < sf.graveyardZones[p].count(); ++i) record(sf.graveyardZones[p].peek(i));
            for (int i = 0; i < sf.banishedZones[p].count(); ++i) record(sf.banishedZones[p].peek(i));
            for (int i = 0; i < sf.sideDeckZones[p].count(); ++i) record(sf.sideDeckZones[p].peek(i));
        }
        for (auto &z : sf.extraMonsterZones) record(z.peek());
    }

    // Remap token pointers held outside the field zones.
    auto sw = [&](Card *&c) {
        auto it = remap.find(c);
        if (it != remap.end()) c = it->second;
    };
    sw(dst.turnState.pending.attacker);
    sw(dst.turnState.pending.target);
    sw(dst.turnState.replayAttacker);
    auto remapSet = [&](std::set<Card *> &s) {
        std::set<Card *> out;
        for (Card *c : s) {
            sw(c);
            out.insert(c);
        }
        s.swap(out);
    };
    remapSet(dst.turnState.attacked);
    remapSet(dst.turnState.flipSummoned);
    remapSet(dst.turnState.positionChanged);
    for (auto &l : dst.chain.links) {
        sw(l.args.target);
        sw(l.args.source);
        for (Card *&m : l.args.materials) sw(m);
    }
    return dst;
}

// One undo step. `duel` is the live state; `cardStates` re-applies the state
// of the shared external cards captured at checkpoint time.
struct DuelSnapshot {
    Duel duel;
    std::map<Card *, cards::CardState> cardStates;
};

inline DuelSnapshot makeSnapshot(const Duel &d) {
    DuelSnapshot s;
    s.duel = cloneDuel(d, &s.cardStates);
    return s;
}

inline void restoreSnapshot(Duel &d, const DuelSnapshot &s) {
    d = cloneDuel(s.duel);  // fresh token clones again (remap inside)
    for (auto &[c, st] : s.cardStates)
        if (c) c->state = st;
}

}  // namespace openjoey::engine