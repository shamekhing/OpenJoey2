#include "support/Fixtures.hpp"

TEST_CASE("TurnManager walks Draw->Standby->Main1->Battle->Main2->End (p.30)",
          "[duel][turn]") {
    DuelProtocol t;
    REQUIRE(t.phase == Phase::Draw);
    REQUIRE_FALSE(t.canAct());  // Draw step is not an action window

    t.nextPhase();
    REQUIRE(t.phase == Phase::Standby);
    t.nextPhase();
    REQUIRE(t.phase == Phase::Main1);
    REQUIRE(t.canAct());
    t.nextPhase();
    REQUIRE(t.phase == Phase::Battle);
    REQUIRE(t.canAct());
    t.nextPhase();
    REQUIRE(t.phase == Phase::Main2);
    REQUIRE(t.canAct());
    t.nextPhase();
    REQUIRE(t.phase == Phase::End);
    REQUIRE_FALSE(t.canAct());
    t.nextPhase();
    REQUIRE(t.phase == Phase::Draw);
    REQUIRE(t.turnNumber == 2);
}

TEST_CASE("Engine setup: opening hand + first-turn skips (p.27)", "[engine][turn]") {
    Duel d;
    Card deck0[7], deck1[7];
    std::vector<Card *> dv0, dv1;
    for (int i = 0; i < 7; ++i) {
        dv0.push_back(mkMon(&deck0[i], 100 + i, 4, 1000 + i, 800));
        dv1.push_back(mkMon(&deck1[i], 200 + i, 4, 900 + i, 700));
    }
    action::SetDeck(d, 0, dv0);
    action::SetDeck(d, 1, dv1);
    REQUIRE(d.lp[0] == DuelConfig::START_LP);
    REQUIRE(d.lp[1] == DuelConfig::START_LP);

    action::DrawOpeningHands(d);
    REQUIRE(d.field.handZones[0].count() == DuelConfig::START_HAND);
    REQUIRE(d.field.handZones[1].count() == DuelConfig::START_HAND);

    // Starting player (P0): no draw, no Battle Phase.
    REQUIRE(action::StartTurn(d).find("turn 1") != std::string::npos);
    REQUIRE(d.field.handZones[0].count() == DuelConfig::START_HAND);  // no draw
    REQUIRE(d.turn.skipBattle);
    d.turn.phase = Phase::Battle;
    REQUIRE_FALSE(action::BattlePhaseOpen(d));
}

TEST_CASE("Engine turn swap + End Phase hand limit of 6 (p.44)", "[engine][turn]") {
    Duel d;
    Card extra[8];
    for (int i = 0; i < 8; ++i) {
        mkMon(&extra[i], 300 + i, 4, 1000, 800);
        d.field.handZones[0].put(&extra[i]);  // 8 in hand = 2 over the limit
    }

    REQUIRE(action::EndTurn(d).find("2 discarded") != std::string::npos);
    REQUIRE(d.field.handZones[0].count() == DuelConfig::HAND_LIMIT);
    REQUIRE(d.turnPlayer == 1);  // players swapped
    REQUIRE(d.turn.turnNumber == 2);
    REQUIRE_FALSE(d.turn.skipBattle);  // skips were turn-1 only
    REQUIRE_FALSE(d.turn.skipDraw);
}

TEST_CASE("Deck-out at the mandatory draw loses the duel (p.44)",
          "[engine][win]") {
    Duel d;
    d.turnPlayer = 1;  // P1 must draw from an empty deck
    d.turn.skipDraw = false;
    REQUIRE(action::DrawForTurn(d).find("deck out") != std::string::npos);
    CHECK(d.result == DuelResult::Player0Win);
    CHECK(d.winReason == WinReason::DeckOut);
    // A decided duel is never overwritten.
    CHECK(action::CheckWinConditions(d) == DuelResult::Player0Win);
}

TEST_CASE("LP depletion: one player at 0 loses, both at 0 is a Draw (p.44)",
          "[engine][win]") {
    SECTION("only P1 at 0") {
        Duel d;
        d.lp[1] = 0;
        CHECK(action::CheckWinConditions(d) == DuelResult::Player0Win);
        CHECK(d.winReason == WinReason::LPDepletion);
    }
    SECTION("both at 0 — Draw") {
        Duel d;
        d.lp[0] = 0;
        d.lp[1] = 0;
        CHECK(action::CheckWinConditions(d) == DuelResult::Draw);
        CHECK(d.winReason == WinReason::LPDepletion);
    }
}
