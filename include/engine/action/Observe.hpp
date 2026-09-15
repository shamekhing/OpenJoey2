#pragma once
// ── act/Observe — read-only state view + legal action space ──────────────────
// The AI seams (foundation ai/Ai.hpp): RL player consumes Observe +
// LegalActions + the act:: functions. Both raylib-free.
#include <array>

#include "engine/action/Battle.hpp"
#include "engine/action/Chain.hpp"
#include "engine/action/Query.hpp"
#include "engine/action/Summon.hpp"

namespace openjoey::engine::action {

struct CardView {
    uint32_t cardId = 0;
    std::string name = "?";
    int atk = 0, def = 0;
    bool faceUp = false, token = false;
};

struct SideView {
    std::vector<CardView> monsters, spellsTraps, hand;
    int handCount = 0, deckCount = 0, extraDeckCount = 0, gyCount = 0, banishedCount = 0;
};

struct StateView {
    int turnPlayer = 0, turnNumber = 0;
    Phase phase = Phase::Draw;
    std::array<int, 2> lp{};
    std::array<SideView, 2> sides;
};

inline CardView MakeView(Card *c, bool known, bool faceUp) {
    CardView cv;
    if (!c) return cv;
    cv.token = c->state.isToken;
    cv.faceUp = faceUp;
    if (!known) return cv;
    cv.cardId = c->id;
    cv.name = c->name;
    cv.atk = c->atk;
    cv.def = c->def;
    return cv;
}

inline StateView Observe(const Duel &d, int viewer) {
    StateView v;
    v.turnPlayer = d.turnPlayer;
    v.turnNumber = d.turn.turnNumber;
    v.phase = d.turn.phase;
    v.lp = d.lp;
    for (int p = 0; p < zone::Field::PLAYERS; ++p) {
        auto &side = v.sides[p];
        bool mine = (p == viewer);
        for (auto &mz : d.field.monsterZones[p])
            if (Card *c = mz.peek()) side.monsters.push_back(MakeView(c, mine || mz.isVisible(), mz.isVisible()));
        for (auto &st : d.field.spellTrapZones[p])
            if (Card *c = st.peek()) side.spellsTraps.push_back(MakeView(c, mine || st.isVisible(), st.isVisible()));
        side.handCount = d.field.handZones[p].count();
        side.deckCount = d.field.deckZones[p].count();
        side.extraDeckCount = d.field.extraDeckZones[p].count();
        side.gyCount = d.field.graveyardZones[p].count();
        side.banishedCount = d.field.banishedZones[p].count();
        if (mine)
            for (int i = 0; i < side.handCount; ++i)
                if (Card *c = d.field.handZones[p].peek(i)) side.hand.push_back(MakeView(c, true, true));
    }
    return v;
}

// Legal action space: verb ids only, no card knowledge. Which specific card
// to act with is the caller's pick (the UI grid / the RL policy).
inline std::vector<ActionSpec> LegalActions(const Duel &d, int player) {
    std::vector<ActionSpec> out;
    if (d.result != DuelResult::Ongoing || player != d.turnPlayer) return out;
    const bool mainPhase = d.canAct();
    auto push = [&](ActionId id) {
        ActionSpec s;
        s.id = id;
        s.speed = 1;
        out.push_back(std::move(s));
    };

    if (mainPhase && CanNormalSummon(d)) {
        bool anyM = false, anyHi = false;
        for (int i = 0; i < d.field.handZones[player].count(); ++i)
            if (Card *c = d.field.handZones[player].peek(i))
                if (c->isMonster()) {
                    anyM = true;
                    if (TributesRequired(c) > 0) anyHi = true;
                }
        if (anyM) {
            push(ActionId::Summon_Normal);
            push(ActionId::Summon_Set);
        }
        if (anyHi) {
            push(ActionId::TributeSummon);
            push(ActionId::TributeSet);
        }
        bool anySet = false, anyUp = false;
        for (auto &mz : d.field.monsterZones[player])
            if (Card *c = mz.peek())
                if (c->state.controller == player) {
                    if (!mz.isVisible()) anySet = !c->state.setThisTurn;
                    else anyUp = !c->state.placedThisTurn && !c->state.setThisTurn && !d.turnState.flipSummoned.count(c) && !d.turnState.positionChanged.count(c);
                }
        if (anySet) push(ActionId::FlipSummon);
        if (anyUp) push(ActionId::ChangeMonsterBattlePosition);
    }
    if (d.turn.phase == Phase::Main1 && !d.turn.skipBattle) push(ActionId::EnterBattlePhase);
    if (d.turn.phase == Phase::Battle)
        for (auto &mz : d.field.monsterZones[player])
            if (Card *c = mz.peek())
                if (CanAttack(d, c)) {
                    push(ActionId::DeclareAttack);
                    break;
                }
    out.push_back([] {
        ActionSpec s;
        s.id = ActionId::EndTurn;
        s.speed = 1;
        return s;
    }());
    return out;
}

}  // namespace openjoey::engine::action
