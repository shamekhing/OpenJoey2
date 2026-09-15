// ── Duel implementation (duel/Duel.cpp) ──────────────────────────────────────
#include "engine/duel/Duel.hpp"

namespace openjoey::engine {

void Duel::seedRng(uint32_t seed) { rng.seed(seed); }

bool Duel::deckBackingMatches(int player, const void *vec) const {
    for (auto &b : deckBackings)
        if (b.first == player) return b.second == vec;  // one recorded backing per player
    return false;                                       // never sealed for this player
}

void Duel::traceBattle(protocol::BattleStep s) {
    battleStep = s;
    battleTrace.push_back(s);
}

uint32_t Duel::stateHash() const {
    uint32_t h = 2166136261u;
    auto mix = [&](uint32_t v) { h ^= v; h *= 16777619u; };
    auto mixCard = [&](const Card *c, int zoneBits) {
        if (!c) { mix(0); return; }
        mix(c->id);
        mix(static_cast<uint32_t>(c->state.controller) | (zoneBits << 8));
        mix(static_cast<uint32_t>(c->state.atkMod) * 31u + static_cast<uint32_t>(c->state.defMod));
        uint32_t ctr = static_cast<uint32_t>(c->state.setThisTurn) | (static_cast<uint32_t>(c->state.placedThisTurn) << 1);
        for (const auto &[name, n] : c->state.counters) ctr += n * 7u;  // name order irrelevant: summed
        mix(ctr);
    };
    mix(static_cast<uint32_t>(lp[0])); mix(static_cast<uint32_t>(lp[1]));
    mix(static_cast<uint32_t>(turn.phase)); mix(static_cast<uint32_t>(turn.turnNumber));
    mix(static_cast<uint32_t>(turnPlayer)); mix(static_cast<uint32_t>(result));
    for (int p = 0; p < zone::Field::PLAYERS; ++p) {
        for (const auto &mz : field.monsterZones[p]) mixCard(mz.peek(), p + 1);
        for (const auto &st : field.spellTrapZones[p]) mixCard(st.peek(), p + 1);
        mixCard(field.fieldSpellZones[p].peek(), p + 1);
        const zone::ZoneStack *stacks[] = {&field.handZones[p], &field.deckZones[p], &field.extraDeckZones[p],
                                           &field.graveyardZones[p], &field.banishedZones[p], &field.sideDeckZones[p]};
        for (const zone::ZoneStack *s : stacks) {
            mix(static_cast<uint32_t>(s->count()));
            for (int i = 0; i < s->count(); ++i) mixCard(s->peek(i), 0);
        }
    }
    for (const auto &t : field.tokens) mixCard(t.get(), 0);
    mix(static_cast<uint32_t>(chain.links.size()));
    for (const auto &l : chain.links) mix(static_cast<uint32_t>(l.id) ^ (static_cast<uint32_t>(l.activator) << 16));
    return h;
}

bool Duel::canAct() const { return turn.canAct(); }

bool Duel::controls(const Card *c, int p) const { return c && c->state.controller == p; }

}  // namespace openjoey::engine
