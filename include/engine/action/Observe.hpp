#pragma once
// ── action/Observe — read-only state view + legal action space ──────────────
// The AI seams (foundation ai/Ai.hpp): RL player consumes Observe +
// LegalActions + the act:: functions. Card reader consumes CardDef →
// vector<ActionSpec> (same role as Catalog.hpp). Both raylib-free.
#include <array>

#include "Catalog.hpp"
#include "engine/action/Battle.hpp"
#include "engine/action/Chains.hpp"
#include "engine/action/State.hpp"
#include "engine/action/Summons.hpp"
#include "engine/action/Support.hpp"

namespace openjoey::engine::action {

struct CardView {
    uint32_t cardId = 0;
    std::string name = "?";
    int atk = 0, def = 0;
    bool faceUp = false, token = false;
};

struct SideView {
    std::vector<CardView> monsters, spellsTraps, hand;
    int handCount = 0, deckCount = 0, extraDeckCount = 0, gyCount = 0,
        banishedCount = 0;
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
            if (Card *c = mz.peek())
                side.monsters.push_back(MakeView(c, mine || mz.isVisible(), mz.isVisible()));
        for (auto &st : d.field.spellTrapZones[p])
            if (Card *c = st.peek())
                side.spellsTraps.push_back(MakeView(c, mine || st.isVisible(), st.isVisible()));
        side.handCount = d.field.handZones[p].count();
        side.deckCount = d.field.deckZones[p].count();
        side.extraDeckCount = d.field.extraDeckZones[p].count();
        side.gyCount = d.field.graveyardZones[p].count();
        side.banishedCount = d.field.banishedZones[p].count();
        if (mine)
            for (int i = 0; i < side.handCount; ++i)
                if (Card *c = d.field.handZones[p].peek(i))
                    side.hand.push_back(MakeView(c, true, true));
    }
    return v;
}

inline std::vector<ActionSpec> LegalActions(const Duel &d, int player) {
    std::vector<ActionSpec> out;
    if (d.result != DuelResult::Ongoing || player != d.turnPlayer) return out;
    const bool mainPhase = d.canAct();
    if (mainPhase && CanNormalSummon(d)) {
        bool anyM = false, anyHi = false;
        for (int i = 0; i < d.field.handZones[player].count(); ++i)
            if (Card *c = d.field.handZones[player].peek(i))
                if (c->isMonster()) {
                    anyM = true;
                    if (TributesRequired(c) > 0) anyHi = true;
                }
        if (anyM) {
            out.push_back({ActionId::Summon_Normal, EffectType::Ignition, 1});
            out.push_back({ActionId::Summon_Set, EffectType::Ignition, 1});
        }
        if (anyHi) {
            out.push_back({ActionId::TributeSummon, EffectType::Ignition, 1});
            out.push_back({ActionId::TributeSet, EffectType::Ignition, 1});
        }
        bool anySet = false, anyUp = false;
        for (auto &mz : d.field.monsterZones[player])
            if (Card *c = mz.peek())
                if (c->state.controller == player) {
                    if (!mz.isVisible())
                        anySet = !c->state.setThisTurn;
                    else
                        anyUp = !c->state.placedThisTurn && !c->state.setThisTurn &&
                                !d.turnState.flipSummoned.count(c) &&
                                !d.turnState.positionChanged.count(c);
                }
        if (anySet) out.push_back({ActionId::FlipSummon, EffectType::Ignition, 1});
        if (anyUp)
            out.push_back({ActionId::ChangeMonsterBattlePosition, EffectType::Ignition, 1});
    }
    if (d.turn.phase == Phase::Main1 && !d.turn.skipBattle)
        out.push_back({ActionId::EnterBattlePhase, EffectType::Ignition, 1});
    if (d.turn.phase == Phase::Battle)
        for (auto &mz : d.field.monsterZones[player])
            if (Card *c = mz.peek())
                if (CanAttack(d, c)) {
                    out.push_back({ActionId::DeclareAttack, EffectType::Ignition, 1});
                    break;
                }
    if (mainPhase) {
        for (int p = 0; p < 2; ++p)
            for (auto &z : d.field.spellTrapZones[p])
                if (Card *c = z.peek())
                    if (findClassicEffect(c->name)) {
                        out.push_back({ActionId::ActivateCardEffect, EffectType::Ignition, 1});
                        p = 2;
                        break;
                    }
        for (auto &mz : d.field.monsterZones[player])
            if (Card *c = mz.peek())
                if (!classicEffectsFor(c->name).empty()) {
                    out.push_back({ActionId::ActivateMonsterEffect, EffectType::Ignition, 1});
                    break;
                }
    }
    out.push_back({ActionId::EndTurn, EffectType::Ignition, 1});
    return out;
}

}  // namespace openjoey::engine::action
