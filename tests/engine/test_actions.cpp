#include "support/Fixtures.hpp"

TEST_CASE("Tokens: spawned by the mat, fight, cease to exist off the field", "[action][token]") {
    Duel d;
    action::StartTurn(d);
    d.turn.phase = Phase::Main1;
    Card *tok = action::SummonToken(d, "Sheep", 500, 500);
    REQUIRE(tok != nullptr);
    CHECK(tok->state.isToken);
    CHECK(d.field.monsterZones[0][0].contains(tok));
    REQUIRE(d.field.tokens.size() == 1);

    // Token battles and dies: erased from the mat, never in the GY.
    Card enemy;
    mkMon(&enemy, 9001, 4, 2000, 100);
    fieldMonster(d, &enemy, 1, 0);
    d.turn.skipBattle = false;  // turn-1 restriction waived for the test
    d.turn.phase = Phase::Battle;
    REQUIRE(action::DeclareAttack(d, tok, &enemy).msg.find(" attacks ") != std::string::npos);
    action::ResolveDamage(d);
    CHECK(d.field.monsterZones[0][0].isEmpty());
    CHECK(d.field.tokens.empty());
    CHECK_FALSE(d.field.graveyardZones[0].contains(tok));
}

TEST_CASE("Equip: bonus applies in battle math and sweeps on destruction", "[action][equip]") {
    Duel d;
    Card hero, blade;
    mkMon(&hero, 9100, 4, 1500, 1200);
    fieldMonster(d, &hero, 0, 0);
    blade.id = 9101;
    blade.name = "PowerBlade";
    blade.attributes.push_back(Attribute::Spell);
    blade.state.owner = blade.state.controller = 0;
    blade.state.bonusAtk = 700;
    d.field.spellTrapZones[0][0].put(&blade);
    d.field.spellTrapZones[0][0].changeVisibility(Visibility::Visible);
    action::StartTurn(d);
    d.turn.phase = Phase::Main1;

    CHECK(action::EquipCard(d, &blade, &hero).msg.find("equips") != std::string::npos);
    CHECK(hero.state.atkMod == 700);
    CHECK(hero.effectiveAtk() == 2200);

    // Effective stats decide battle: 2200 beats 2000 where 1500 would lose.
    Card enemy;
    mkMon(&enemy, 9102, 4, 2000, 100);
    fieldMonster(d, &enemy, 1, 0);
    d.turn.skipBattle = false;  // turn-1 restriction waived for the test
    d.turn.phase = Phase::Battle;
    REQUIRE(action::DeclareAttack(d, &hero, &enemy).msg.find(" attacks ") != std::string::npos);
    action::ResolveDamage(d);
    CHECK(d.field.monsterZones[1][0].isEmpty());
    CHECK(d.field.monsterZones[0][0].contains(&hero));

    // Destroying the host sweeps the equip into the GY, bonus rolled back.
    action::PlaceCounterD(d, &hero, "focus", 2);
    CHECK(hero.state.counters.at("focus") == 2);
    CHECK(action::RemoveCounterD(d, &hero, "focus", 1) == 1);
    CHECK(action::RemoveCounterD(d, &hero, "focus", 5) == 1);  // clamped to what exists
    CHECK(action::RemoveCounterD(d, &hero, "focus", 1) == 0);
}

TEST_CASE("Equips detach with exact rollback when the equip itself dies", "[action][equip]") {
    Duel d;
    Card hero, blade;
    mkMon(&hero, 9110, 4, 1500, 1200);
    fieldMonster(d, &hero, 0, 0);
    blade.id = 9111;
    blade.name = "Blade";
    blade.attributes.push_back(Attribute::Spell);
    blade.state.owner = blade.state.controller = 0;
    blade.state.bonusAtk = 500;
    d.field.spellTrapZones[0][0].put(&blade);
    action::StartTurn(d);
    d.turn.phase = Phase::Main1;
    REQUIRE(action::EquipCard(d, &blade, &hero).msg.find("equips") != std::string::npos);
    CHECK(hero.effectiveAtk() == 2000);
    CHECK(action::UnequipCard(d, &blade).msg.find("unequipped") != std::string::npos);
    CHECK(hero.state.atkMod == 0);
    CHECK(hero.effectiveAtk() == 1500);
    CHECK(hero.state.equippedCards.empty());
}

TEST_CASE("Search/excavate and the special-summon family", "[action][special]") {
    Duel d;
    Card deckCard, gyCard;
    mkMon(&deckCard, 9200, 4, 1000, 800);
    deckCard.name = "Searchable";
    d.field.deckZones[0].put(&deckCard);
    mkMon(&gyCard, 9201, 4, 1200, 900);
    gyCard.name = "Revivable";
    d.field.graveyardZones[0].put(&gyCard);
    action::StartTurn(d);
    d.turn.phase = Phase::Main1;

    CHECK(action::SearchDeck(d, [](const Card &c) { return c.name == "Searchable"; }) == 1);
    CHECK(d.field.handZones[0].contains(&deckCard));
    CHECK(action::SearchDeck(d, [](const Card &) { return false; }) == 0);

    auto revealed = action::Excavate(d, 2);
    CHECK(revealed.empty());  // deck empty now — nothing to reveal

    // Special summon from the graveyard, face-down DEF this time.
    CHECK(action::SpecialSummon(d, &gyCard, /*faceDown=*/true).msg.find("special summons") != std::string::npos);
    CHECK(d.field.monsterZones[0][0].contains(&gyCard));
    CHECK_FALSE(d.field.monsterZones[0][0].isVisible());
}

TEST_CASE("Tribute Set + CardEffect win + observe/legalActions seams", "[action][ai]") {
    Duel d;
    Card big, small;
    mkMon(&big, 9300, 6, 2200, 1800);
    big.name = "TributeTarget";
    d.field.handZones[0].put(&big);
    mkMon(&small, 9301, 4, 1000, 900);
    fieldMonster(d, &small, 0, 0);
    action::StartTurn(d);
    d.turn.phase = Phase::Main1;

    CHECK(action::SummonTribute(d, &big, {&small}, true).msg.find("tribute summons") != std::string::npos);
    CHECK(d.field.monsterZones[0][0].contains(&big));
    CHECK_FALSE(d.field.monsterZones[0][0].isVisible());

    // observe(): a viewer sees own faces, opponent set cards as hidden backs.
    auto view = action::Observe(d, 0);
    CHECK(view.lp[0] == DuelConfig::START_LP);
    CHECK(view.sides[0].monsters[0].faceUp == false);          // the tribute SET
    CHECK(view.sides[0].monsters[0].name == "TributeTarget");  // own set: known

    // legalActions(): the action space is non-empty and typed.
    auto acts = action::LegalActions(d, 0);
    CHECK_FALSE(acts.empty());
    bool hasEnd = false;
    for (auto &a : acts)
        if (a.id == ActionId::EndTurn) hasEnd = true;
    CHECK(hasEnd);

    // Card-effect win: decided duels are never overwritten by LP checks.
    action::SetResult(d, DuelResult::Player0Win, WinReason::CardEffectWin);
    CHECK(d.result == DuelResult::Player0Win);
    CHECK(d.winReason == WinReason::CardEffectWin);
    action::CheckWinConditions(d);
    CHECK(d.result == DuelResult::Player0Win);
}

TEST_CASE("perform() realizes EVERY ActionId — no action is unimplemented", "[actions][complete]") {
    Duel d;
    action::StartTurn(d);
    d.turn.phase = Phase::Main1;

    // Spot-checks with controlled state (before the exhaustive loop mutates it):
    Card gy, gy2;
    mkMon(&gy, 9400, 4, 1000, 800);
    gy.name = "ReviveMe";
    d.field.graveyardZones[0].put(&gy);
    mkMon(&gy2, 9401, 3, 900, 700);
    gy2.name = "GyFiller";
    d.field.graveyardZones[0].put(&gy2);
    d.turn.skipBattle = false;
    d.turn.phase = Phase::Main1;
    ActionArgs a;
    a.target = &gy;
    CHECK(action::Perform(d, ActionId::SpecialSummonFromGraveyard, a).msg.find("special summons") != std::string::npos);
    CHECK(d.field.monsterZones[0][0].contains(&gy));
    // face-down special summon this turn: Flip Summon correctly illegal now
    CHECK(action::Perform(d, ActionId::CannotFlipSummonSameTurn, a).msg.find("cannot flip summon now") != std::string::npos);
    CHECK(action::Perform(d, ActionId::CheckHandSize).msg.find("hand size") != std::string::npos);
    CHECK(action::Perform(d, ActionId::SynchroSummon).msg.find("not legal in the classic") != std::string::npos);
    CHECK(action::Perform(d, ActionId::ViewGraveyard).msg.find("GyFiller") != std::string::npos);

    // ── Regression: CardEffectWin credits the ACTIVATOR (was hardcoded P0) ──
    d.result = DuelResult::Ongoing;
    d.turnPlayer = 1;
    CHECK(action::Perform(d, ActionId::CardEffectWin).ok);
    CHECK(d.result == DuelResult::Player1Win);
    d.result = DuelResult::Ongoing;
    d.turnPlayer = 0;

    // ── Regression: tokens carry caller-supplied stats (was hardcoded 0/0) ──
    {
        ActionArgs ta;
        ta.atk = 1500;
        ta.def = 1200;
        CHECK(action::Perform(d, ActionId::Summon_Token, ta).ok);
        REQUIRE_FALSE(d.field.tokens.empty());
        CHECK(d.field.tokens.back()->atk == 1500);
        CHECK(d.field.tokens.back()->def == 1200);
    }

    // ── Regression: discard uses the safe move primitive — the card must be
    // in the Graveyard and no longer in the hand (the old put-then-remove
    // left it in two zones at once). ─────────────────────────────────────────
    {
        Card hand;
        mkMon(&hand, 9500, 2, 800, 600);
        hand.name = "DiscardMe";
        d.field.handZones[0].put(&hand);
        ActionArgs da;
        da.target = &hand;
        CHECK(action::Perform(d, ActionId::SelectAndDiscard, da).ok);
        CHECK_FALSE(d.field.handZones[0].contains(&hand));
        CHECK(d.field.graveyardZones[0].contains(&hand));
    }

    // Iterate the ENTIRE enum. The dispatcher must handle every id explicitly:
    // the fallthrough sentinel and the chain resolver's old silent no-op are
    // hard failures now, not strings that merely happen to be non-empty. The
    // old loop only rejected two literals Perform never returned, so an
    // unhandled id passed vacuously.
    for (int v = 1; v <= static_cast<int>(ActionId::ZoneBecomesPendulumZone); ++v) {
        // v=0 is None (the idle id, not an action) — excluded above.
        auto id = static_cast<ActionId>(v);
        ActionArgs args;
        args.target = nullptr;
        args.n = 1;
        const ActionResult r = action::Perform(d, id, args);
        INFO("ActionId value " << v << " -> \"" << r.msg << "\"");
        REQUIRE(r.msg.find("not handled") == std::string::npos);
        REQUIRE(r.msg.find("NOT IMPLEMENTED") == std::string::npos);
        REQUIRE_FALSE(r.msg.empty());
    }
}
