#include "support/Fixtures.hpp"

TEST_CASE("Engine Normal Summon: once per turn, Main Phase only (p.24)", "[engine][summon]") {
    Duel d;
    Card m, m2;
    mkMon(&m, 400, 4, 1800, 1200);
    mkMon(&m2, 401, 4, 1500, 1000);
    d.field.handZones[0].put(&m);
    d.field.handZones[0].put(&m2);
    d.turn.phase = Phase::Main1;

    REQUIRE(action::SummonNormal(d, &m).find("summons") != std::string::npos);
    REQUIRE(d.field.monsterZones[0][0].contains(&m));
    REQUIRE(d.field.monsterZones[0][0].isVisible());
    REQUIRE(d.field.monsterZones[0][0].position() == Orientation::Vertical);
    REQUIRE(m.state.placedThisTurn);

    // Once per turn.
    REQUIRE(action::SummonNormal(d, &m2).find("no normal summon") != std::string::npos);
    REQUIRE_FALSE(d.field.monsterZones[0][1].contains(&m2));

    // Not in the Battle Phase.
    d.turn.phase = Phase::Battle;
    REQUIRE_FALSE(action::CanNormalSummon(d));
}

TEST_CASE("Engine Normal Set: face-down DEF, limited visibility (p.24)", "[engine][summon]") {
    Duel d;
    Card m;
    mkMon(&m, 410, 4, 1800, 1200);
    d.field.handZones[0].put(&m);
    d.turn.phase = Phase::Main1;

    REQUIRE(action::SummonSet(d, &m).find("sets") != std::string::npos);
    REQUIRE(d.field.monsterZones[0][0].contains(&m));
    REQUIRE_FALSE(d.field.monsterZones[0][0].isVisible());  // face-down
    REQUIRE(d.field.monsterZones[0][0].position() == Orientation::Horizontal);
    REQUIRE(m.state.setThisTurn);
}

TEST_CASE("Engine Tribute Summon: level→tribute count, tributes to GY (p.23)", "[engine][summon]") {
    Duel d;
    Card lv4, lv5, lv7, t1, t2;
    mkMon(&lv4, 420, 4, 1800, 1200);
    mkMon(&lv5, 421, 5, 1900, 1300);
    mkMon(&lv7, 422, 7, 2500, 2000);
    mkMon(&t1, 423, 4, 1000, 800);
    mkMon(&t2, 424, 4, 1100, 900);

    REQUIRE(action::TributesRequired(&lv4) == 0);
    REQUIRE(action::TributesRequired(&lv5) == 1);
    REQUIRE(action::TributesRequired(&lv7) == 2);

    d.field.handZones[0].put(&lv7);  // tributeSummon summons from the hand
    d.turn.phase = Phase::Main1;
    // Wrong tribute count is rejected and does NOT consume the summon.
    REQUIRE(action::SummonTribute(d, &lv7, {}, false).find("exactly 2") != std::string::npos);
    REQUIRE(action::CanNormalSummon(d));

    fieldMonster(d, &t1, 0, 0);
    fieldMonster(d, &t2, 0, 1);
    REQUIRE(action::SummonTribute(d, &lv7, {&t1, &t2}, false).find("summons") != std::string::npos);
    REQUIRE(d.field.findCard(&t1).first == &d.field.graveyardZones[0]);
    REQUIRE(d.field.findCard(&t2).first == &d.field.graveyardZones[0]);
    REQUIRE(d.field.monsterZones[0][0].contains(&lv7));
    REQUIRE_FALSE(action::CanNormalSummon(d));  // consumed
}

TEST_CASE("Engine Flip Summon: not the Set turn, becomes face-up ATK (p.25)",
          "[engine][summon]") {
    Duel d;
    Card m;
    mkMon(&m, 430, 4, 1600, 1400);
    d.field.handZones[0].put(&m);
    d.turn.phase = Phase::Main1;
    REQUIRE(action::SummonSet(d, &m).find("sets") != std::string::npos);

    // Same turn: illegal.
    REQUIRE(action::FlipSummon(d, &m).find("the turn it was Set") != std::string::npos);

    // Two turn swaps later it is P0's turn again and the Set-turn flag has
    // expired — Flip Summon now works and turns the monster face-up ATK.
    REQUIRE(action::EndTurn(d).find("hand limit OK") != std::string::npos);
    REQUIRE(action::EndTurn(d).find("hand limit OK") != std::string::npos);
    REQUIRE(d.turnPlayer == 0);
    d.turn.phase = Phase::Main1;  // endTurn() leaves the Draw phase set
    std::string msg = action::FlipSummon(d, &m);
    REQUIRE(msg.find("Flip Summons") != std::string::npos);
    REQUIRE(d.field.monsterZones[0][0].isVisible());
    REQUIRE(d.field.monsterZones[0][0].position() == Orientation::Vertical);

    // It arrived this turn (via flip): no position change.
    REQUIRE(action::ChangePosition(d, &m).find("the turn it arrived") != std::string::npos);
}

TEST_CASE("Engine Flip effect: trigger is announced (p.25)", "[engine][summon]") {
    Duel d;
    Card m;
    mkMon(&m, 431, 4, 1200, 1000);
    m.name = "Man-Eater Bug";  // flip effect resolves from the classic catalog
    d.field.handZones[0].put(&m);
    d.turn.phase = Phase::Main1;
    action::SummonSet(d, &m);
    action::EndTurn(d);
    action::EndTurn(d);
    d.turn.phase = Phase::Main1;  // endTurn() leaves the Draw phase set
    REQUIRE(action::FlipSummon(d, &m).find("Flip effect triggers") != std::string::npos);
}

TEST_CASE("Engine position change: once per turn, not on arrival turn (p.26)",
          "[engine][summon]") {
    Duel d;
    Card m;
    mkMon(&m, 432, 4, 1800, 1200);
    fieldMonster(d, &m, 0, 0);
    d.turn.phase = Phase::Main1;

    m.state.placedThisTurn = true;  // simulate: summoned this turn
    REQUIRE(action::ChangePosition(d, &m).find("the turn it arrived") != std::string::npos);
    m.state.placedThisTurn = false;

    REQUIRE(action::ChangePosition(d, &m).find("switches to DEF") != std::string::npos);
    REQUIRE(d.field.monsterZones[0][0].position() == Orientation::Horizontal);

    // Once per turn.
    REQUIRE(action::ChangePosition(d, &m).find("already changed") != std::string::npos);

    // Face-down monsters are Flip Summoned, not position-changed.
    d.field.monsterZones[0][0].changeVisibility(Visibility::Limited);
    REQUIRE(action::ChangePosition(d, &m).find("Flip Summoned") != std::string::npos);
}

// ── Battle Phase (p.34–38) ───────────────────────────────────────────────────

TEST_CASE("Special Summon may choose face-up DEF (p.25)", "[engine][summon]") {
    Duel d;
    Card m;
    mkMon(&m, 460, 4, 1200, 2000);
    d.field.handZones[0].put(&m);
    d.turnPlayer = 0;
    d.turn.turnNumber = 2;
    d.turn.phase = Phase::Main1;

    REQUIRE(action::SpecialSummon(d, &m, action::SpecialPose::DefUp)
                .find("face-up DEF") != std::string::npos);
    REQUIRE(d.field.monsterZones[0][0].contains(&m));
    REQUIRE(d.field.monsterZones[0][0].isVisible());
    REQUIRE(d.field.monsterZones[0][0].position() == Orientation::Horizontal);
    // Face-up DEF monsters cannot attack.
    d.turn.phase = Phase::Battle;
    REQUIRE_FALSE(action::CanAttack(d, &m));
}

TEST_CASE("Resolver: position changes and effect flips (classic zone ops)",
          "[resolver][classic]") {
    Duel d;
    Card m1, m2;
    fieldMonster(d, mkMon(&m1, 301, 4, 1400, 1200), 0, 0);
    Card *setMon = mkMon(&m2, 302, 3, 800, 900);
    setMon->state.owner = setMon->state.controller = 0;
    d.field.monsterZones[0][1].put(setMon);
    d.field.monsterZones[0][1].changeVisibility(Visibility::Limited);
    d.field.monsterZones[0][1].changeOrientation(Orientation::Horizontal);

    SECTION("ATK -> DEF -> ATK via the resolver") {
        ActionArgs a;
        a.target = &m1;
        CHECK(action::Perform(d, ActionId::Pos_ChangeAToDef, a) ==
              std::string("switched to Defense Position."));
        CHECK(d.field.monsterZones[0][0].position() == Orientation::Horizontal);
        ActionArgs b;
        b.target = &m1;
        CHECK(action::Perform(d, ActionId::Pos_ChangeDefToAtk, b)
                  .find("Attack Position") != std::string::npos);
        CHECK(d.field.monsterZones[0][0].position() == Orientation::Vertical);
    }
    SECTION("Summon_Flip turns a set monster face-up") {
        ActionArgs a;
        a.target = setMon;
        CHECK(action::Perform(d, ActionId::Summon_Flip, a)
                  .find("flipped face-up") != std::string::npos);
        CHECK(d.field.monsterZones[0][1].position() == Orientation::Vertical);
        CHECK(d.field.monsterZones[0][1].isVisible());
    }
    SECTION("position change on a non-monster target fails") {
        ActionArgs a;
        a.target = nullptr;
        CHECK(action::Perform(d, ActionId::Pos_ChangeAToDef, a)
                  .find("face-up monster") != std::string::npos);
    }
}

TEST_CASE("Classic scope: Synchro/Xyz summons are rejected, not TODO",
          "[resolver][classic]") {
    Duel d;
    CHECK(action::Perform(d, ActionId::Summon_Synchro)
              .find("not legal in the classic format") != std::string::npos);
    CHECK(action::Perform(d, ActionId::Summon_Xyz)
              .find("not legal in the classic format") != std::string::npos);
}

TEST_CASE("Fusion Summon: Extra Deck -> EMZ, materials -> GY (p.20)",
          "[engine][fusion]") {
    Card fu, mat1, mat2;
    mkMon(&fu, 451, 6, 2600, 2000);
    fu.name = "FlameSwordsman";
    mkMon(&mat1, 452, 4, 1800, 1500);
    mkMon(&mat2, 453, 4, 1600, 1200);

    SECTION("legal fusion summon") {
        Duel d;
        fieldMonster(d, &mat1, 0, 0);
        fieldMonster(d, &mat2, 0, 1);
        fu.state.owner = fu.state.controller = 0;
        d.field.extraDeckZones[0].put(&fu);
        action::StartTurn(d);
        d.turn.phase = Phase::Main1;

        std::string msg = action::FusionSummon(d, &fu, {&mat1, &mat2});
        CHECK(msg.find("Fusion Summons") != std::string::npos);
        CHECK(d.field.extraDeckZones[0].isEmpty());
        CHECK(d.field.extraMonsterZones[0].contains(&fu));
        CHECK(d.field.graveyardZones[0].contains(&mat1));
        CHECK(d.field.graveyardZones[0].contains(&mat2));
        // A Fusion Summon costs no Normal Summon (p.20).
        CHECK(action::CanNormalSummon(d));
    }
    SECTION("needs at least two materials") {
        Duel d;
        fieldMonster(d, &mat1, 0, 0);
        fu.state.owner = fu.state.controller = 0;
        d.field.extraDeckZones[0].put(&fu);
        action::StartTurn(d);
        d.turn.phase = Phase::Main1;
        CHECK(action::FusionSummon(d, &fu, {&mat1})
                  .find("at least 2 material") != std::string::npos);
        CHECK(d.field.extraDeckZones[0].contains(&fu));  // untouched
    }
    SECTION("materials must be your own monsters (field or hand, p.22)") {
        Duel d;
        fieldMonster(d, &mat1, 1, 0);  // opponent-controlled material
        fu.state.owner = fu.state.controller = 0;
        d.field.extraDeckZones[0].put(&fu);
        action::StartTurn(d);
        d.turn.phase = Phase::Main1;
        CHECK(action::FusionSummon(d, &fu, {&mat1, &mat2})
                  .find("must be your own monsters") != std::string::npos);
        CHECK(d.field.extraDeckZones[0].contains(&fu));
    }
    SECTION("hand materials are legal (p.22)") {
        Duel d;
        fieldMonster(d, &mat1, 0, 0);
        fu.state.owner = fu.state.controller = 0;
        d.field.extraDeckZones[0].put(&fu);
        d.field.handZones[0].put(&mat2);  // second material in hand
        mat2.state.owner = mat2.state.controller = 0;
        action::StartTurn(d);
        d.turn.phase = Phase::Main1;
        CHECK(action::FusionSummon(d, &fu, {&mat1, &mat2})
                  .find("Fusion Summons") != std::string::npos);
        CHECK(!d.field.extraDeckZones[0].contains(&fu));  // summoned to EMZ
        CHECK(!d.field.handZones[0].contains(&mat2));     // material -> GY
        CHECK(d.field.graveyardZones[0].contains(&mat2));
    }
    SECTION("material in a non-monster zone is rejected") {
        Duel d;
        fieldMonster(d, &mat1, 0, 0);
        fu.state.owner = fu.state.controller = 0;
        d.field.extraDeckZones[0].put(&fu);
        d.field.graveyardZones[0].put(&mat2);  // GY is not a valid source
        action::StartTurn(d);
        d.turn.phase = Phase::Main1;
        CHECK(action::FusionSummon(d, &fu, {&mat1, &mat2})
                  .find("field or in your hand") != std::string::npos);
    }
    SECTION("monster must be in the Extra Deck") {
        Duel d;
        fieldMonster(d, &mat1, 0, 0);
        fieldMonster(d, &mat2, 0, 1);
        action::StartTurn(d);
        d.turn.phase = Phase::Main1;
        CHECK(action::FusionSummon(d, &fu, {&mat1, &mat2})
                  .find("Extra Deck") != std::string::npos);
    }
}

TEST_CASE("Ritual Summon: hand -> MMZ, tribute levels >= level (p.21)",
          "[engine][ritual]") {
    Card rm, mat1, mat2, mat3;
    mkMon(&rm, 461, 7, 2500, 2100);
    rm.name = "BlackLuster";
    mkMon(&mat1, 462, 4, 1200, 900);
    mkMon(&mat2, 463, 4, 1400, 1000);
    mkMon(&mat3, 464, 2, 800, 700);

    SECTION("legal ritual summon (field tributes)") {
        Duel d;
        fieldMonster(d, &mat1, 0, 0);
        fieldMonster(d, &mat2, 0, 1);
        rm.state.owner = rm.state.controller = 0;
        d.field.handZones[0].put(&rm);
        action::StartTurn(d);
        d.turn.phase = Phase::Main1;

        std::string msg = action::RitualSummon(d, &rm, {&mat1, &mat2});  // 4+4 >= 7
        CHECK(msg.find("Ritual Summons") != std::string::npos);
        CHECK(d.field.handZones[0].isEmpty());
        CHECK(d.field.monsterZones[0][2].contains(&rm));
        CHECK(d.field.monsterZones[0][2].isVisible());
        CHECK(d.field.graveyardZones[0].contains(&mat1));
        CHECK(d.field.graveyardZones[0].contains(&mat2));
        // A Ritual Summon costs no Normal Summon (p.21).
        CHECK(action::CanNormalSummon(d));
    }
    SECTION("tribute levels below the ritual level are rejected") {
        Duel d;
        fieldMonster(d, &mat3, 0, 0);  // level 2 < 7
        rm.state.owner = rm.state.controller = 0;
        d.field.handZones[0].put(&rm);
        action::StartTurn(d);
        d.turn.phase = Phase::Main1;
        CHECK(action::RitualSummon(d, &rm, {&mat3})
                  .find("below level") != std::string::npos);
        CHECK(d.field.handZones[0].contains(&rm));  // untouched
    }
    SECTION("no tributes at all is rejected") {
        Duel d;
        rm.state.owner = rm.state.controller = 0;
        d.field.handZones[0].put(&rm);
        action::StartTurn(d);
        d.turn.phase = Phase::Main1;
        CHECK(action::RitualSummon(d, &rm, {}).find("at least 1") !=
              std::string::npos);
    }
    SECTION("monster must be in hand") {
        Duel d;
        fieldMonster(d, &mat1, 0, 0);
        fieldMonster(d, &mat2, 0, 1);
        rm.state.owner = rm.state.controller = 0;
        d.field.deckZones[0].put(&rm);
        action::StartTurn(d);
        d.turn.phase = Phase::Main1;
        CHECK(action::RitualSummon(d, &rm, {&mat1, &mat2})
                  .find("in your hand") != std::string::npos);
    }
}
