#include "engine/action/Observe.hpp"

namespace openjoey::engine::action {

CardView MakeView(Card *c, bool known, bool faceUp) {
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

StateView Observe(const Duel &d, int viewer) {
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

std::vector<ActionSpec> LegalActions(const Duel &d, int player) {
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
