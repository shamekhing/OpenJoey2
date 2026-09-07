#include "support/Fixtures.hpp"

// ── from tests.cpp lines 651,686 ──
TEST_CASE("Engine position change: an attacked monster cannot switch (p.36)",
          "[engine][summon][battle]") {
    BattleFix fix(2000, 500, 800);
    Card &a = fix.a;
    Duel &d = fix.d;

    REQUIRE(action::DeclareAttack(d, &a, &fix.t).find("attacks") != std::string::npos);
    REQUIRE(action::ResolveDamage(d).find("destroys") != std::string::npos);

    // p.36: cannot change position in Main Phase 2 after attacking.
    d.turn.phase = Phase::Main2;
    REQUIRE(action::ChangePosition(d, &a).find("after attacking") != std::string::npos);
    REQUIRE(d.field.monsterZones[0][0].position() == Orientation::Vertical);
    REQUIRE_FALSE(action::CanChangePosition(d, &a));
}

TEST_CASE("Replay: re-declaring with a DIFFERENT monster locks the first (p.37)",
          "[engine][battle]") {
    BattleFix fix(2000, 500, 800);
    Card a2;
    mkMon(&a2, 9003, 4, 1500, 900);
    fieldMonster(fix.d, &a2, 0, 1);

    REQUIRE(action::DeclareAttack(fix.d, &fix.a, &fix.t).find("attacks") !=
            std::string::npos);
    // Replay trigger: the attack target leaves the field before the Damage Step.
    action::MoveDestroyToGY(fix.d.field, &fix.t);
    REQUIRE_FALSE(action::ConfirmAttack(fix.d));
    REQUIRE(action::CanAttack(fix.d, &fix.a)); // refund: may still re-declare

    // Re-declare with a different monster -> the first attacker is locked.
    REQUIRE(action::DeclareAttack(fix.d, &a2, nullptr).find("direct") !=
            std::string::npos);
    REQUIRE_FALSE(action::CanAttack(fix.d, &fix.a));
}


// ── from tests.cpp lines 749,923 ──
TEST_CASE("Undo: snapshot/restore rolls back a summon, incl. tokens",
          "[engine][undo]") {
    Duel d;
    Engine e(d);
    Card deck[3];
    std::vector<Card *> dv;
    for (int i = 0; i < 3; ++i)
        dv.push_back(mkMon(&deck[i], 810 + i, 4, 1000, 800));
    e.setDeck(0, dv);
    e.sealDeckBacking(0, &dv);
    d.field.handZones[0].put(&deck[0]);
    d.turnPlayer = 0;
    d.turn.turnNumber = 2;
    d.turn.phase = Phase::Main1;

    e.checkpoint();
    REQUIRE(action::SummonNormal(d, &deck[0]).find("summons") != std::string::npos);
    REQUIRE(d.field.monsterZones[0][0].contains(&deck[0]));
    REQUIRE(d.turnState.normalSummonUsed);
    REQUIRE_FALSE(action::CanNormalSummon(d));

    REQUIRE(e.undo());
    REQUIRE(d.field.handZones[0].contains(&deck[0]));  // back in hand
    REQUIRE(d.field.monsterZones[0][0].isEmpty());
    REQUIRE_FALSE(d.turnState.normalSummonUsed);
    REQUIRE(action::CanNormalSummon(d));               // budget restored
    REQUIRE_FALSE(e.undo());                            // nothing older

    // Tokens survive a snapshot round-trip with remapped pointers.
    e.checkpoint();
    Card *tok = action::SummonToken(d, "Sheep", 0, 0);
    REQUIRE(tok != nullptr);
    REQUIRE(e.undo());
    REQUIRE(d.field.tokens.empty());                    // token erased by rollback
    REQUIRE(d.field.monsterZones[0][0].isEmpty());
}

TEST_CASE("Engine battle entry guards (p.34-35)", "[engine][battle]") {
    SECTION("Battle Phase skipped -> nothing can attack") {
        BattleFix f{1800, 1400};
        f.d.turn.skipBattle = true;
        REQUIRE_FALSE(action::CanAttack(f.d, &f.a));
        REQUIRE(action::DeclareAttack(f.d, &f.a, &f.t).find("attack not possible") !=
                std::string::npos);
    }
    SECTION("DEF-position monsters cannot attack") {
        BattleFix f{1800, 1400};
        f.d.field.monsterZones[0][0].changeOrientation(Orientation::Horizontal);
        REQUIRE_FALSE(action::CanAttack(f.d, &f.a));
    }
    SECTION("Opponent's monster cannot attack on your turn") {
        BattleFix f{1800, 1400};
        REQUIRE_FALSE(action::CanAttack(f.d, &f.t));
    }
    SECTION("Direct attack requires an empty opponent field (p.34)") {
        BattleFix f{1800, 1400};
        REQUIRE(action::DeclareAttack(f.d, &f.a, nullptr).find(
                    "empty opponent field") != std::string::npos);
    }
}

TEST_CASE("Engine attack declaration + held-open state (p.35)", "[engine][battle]") {
    BattleFix f{1800, 1400};
    REQUIRE(action::DeclareAttack(f.d, &f.a, &f.t).find("attacks M9002") !=
            std::string::npos);

    // Only one attack held open at a time.
    REQUIRE(action::DeclareAttack(f.d, &f.a, &f.t).find("already held open") !=
            std::string::npos);

    // Calling the attack off clears it.
    action::CancelAttack(f.d);
    REQUIRE_FALSE(action::ConfirmAttack(f.d));

    // Own monster is not a legal target.
    Card mine;
    mkMon(&mine, 9003, 4, 500, 500);
    fieldMonster(f.d, &mine, 0, 2);
    REQUIRE(action::DeclareAttack(f.d, &f.a, &mine).find("invalid attack target") !=
            std::string::npos);
}

TEST_CASE("Engine replay: state change cancels the held attack (p.37)",
          "[engine][replay]") {
    BattleFix f{1800, 1400};
    REQUIRE(action::DeclareAttack(f.d, &f.a, &f.t).find("attacks") != std::string::npos);

    // Target left the field-side of the replay contract: make it "no longer
    // an opponent's monster" (e.g. it changed controllers / left play).
    f.t.state.controller = 0;

    // resolveDamage() itself detects the replay and cancels the held attack.
    REQUIRE(action::ResolveDamage(f.d).find("replay! attack cancelled") !=
            std::string::npos);

    // The attack was never committed: the attacker may still attack.
    REQUIRE(action::CanAttack(f.d, &f.a));
}

// ── Damage Step math (p.38) ──────────────────────────────────────────────────

TEST_CASE("Engine damage: ATK vs ATK (p.38)", "[engine][battle]") {
    SECTION("attacker stronger: defender destroyed, difference pierces") {
        BattleFix f{1800, 1400};
        REQUIRE(action::DeclareAttack(f.d, &f.a, &f.t).find("attacks") !=
                std::string::npos);
        action::ResolveDamage(f.d);
        CHECK(f.d.lp[1] == DuelConfig::START_LP - 400);
        CHECK(f.d.field.monsterZones[1][1].isEmpty());        // defender destroyed
        CHECK_FALSE(f.d.field.monsterZones[0][0].isEmpty());  // attacker survives
        CHECK(f.d.field.graveyardZones[1].contains(&f.t));    // sent to GY
        CHECK_FALSE(action::CanAttack(f.d, &f.a));                   // attack committed
    }
    SECTION("attacker weaker: attacker destroyed, difference rebounds") {
        BattleFix f{1200, 1800};
        action::DeclareAttack(f.d, &f.a, &f.t);
        action::ResolveDamage(f.d);
        CHECK(f.d.lp[0] == DuelConfig::START_LP - 600);
        CHECK(f.d.field.monsterZones[0][0].isEmpty());
        CHECK_FALSE(f.d.field.monsterZones[1][1].isEmpty());
    }
    SECTION("tie: both destroyed, no LP damage") {
        BattleFix f{1500, 1500};
        action::DeclareAttack(f.d, &f.a, &f.t);
        action::ResolveDamage(f.d);
        CHECK(f.d.field.monsterZones[0][0].isEmpty());
        CHECK(f.d.field.monsterZones[1][1].isEmpty());
        CHECK(f.d.lp[0] == DuelConfig::START_LP);
        CHECK(f.d.lp[1] == DuelConfig::START_LP);
    }
}

TEST_CASE("Engine damage: ATK vs DEF (p.38)", "[engine][battle]") {
    SECTION("ATK > DEF: defender destroyed, no damage dealt") {
        BattleFix f{1800, 1000, 1000};
        f.d.field.monsterZones[1][1].changeOrientation(Orientation::Horizontal);
        action::DeclareAttack(f.d, &f.a, &f.t);
        action::ResolveDamage(f.d);
        CHECK(f.d.field.monsterZones[1][1].isEmpty());
        CHECK(f.d.lp[0] == DuelConfig::START_LP); // attacker takes nothing
        CHECK(f.d.lp[1] == DuelConfig::START_LP);
    }
    SECTION("ATK < DEF: rebound damages the attacker's controller") {
        BattleFix f{1800, 1000, 2000};
        f.d.field.monsterZones[1][1].changeOrientation(Orientation::Horizontal);
        action::DeclareAttack(f.d, &f.a, &f.t);
        action::ResolveDamage(f.d);
        CHECK(f.d.lp[0] == DuelConfig::START_LP - 200);
        CHECK_FALSE(f.d.field.monsterZones[0][0].isEmpty()); // attacker survives
        CHECK_FALSE(f.d.field.monsterZones[1][1].isEmpty());
    }
    SECTION("face-down defender is flipped at the Damage Step but STAYS DEF") {
        BattleFix f{1800, 1000, 1000};
        // Set face-down defense directly (flip() is the canonical face-up
        // flip — Horizontal+Limited → Vertical+Visible — not a "set" op).
        f.d.field.monsterZones[1][1].changeOrientation(Orientation::Horizontal);
        f.d.field.monsterZones[1][1].changeVisibility(Visibility::Limited);
        REQUIRE(action::DeclareAttack(f.d, &f.a, &f.t).find("attacks") !=
                std::string::npos);
        action::ResolveDamage(f.d);
        // Flipped face-up for damage calculation, but orientation stayed DEF.
        CHECK(f.d.field.monsterZones[1][1].isVisible());
        CHECK(f.d.field.monsterZones[1][1].position() ==
              Orientation::Horizontal);
        CHECK(f.d.field.monsterZones[1][1].isEmpty());        // 1800 > 1000: destroyed
        CHECK(f.d.lp[0] == DuelConfig::START_LP);
        CHECK(f.d.lp[1] == DuelConfig::START_LP);                 // DEF: no damage
    }
    SECTION("ATK == DEF: nothing happens") {
        BattleFix f{1500, 1000, 1500};
        f.d.field.monsterZones[1][1].changeOrientation(Orientation::Horizontal);
        action::DeclareAttack(f.d, &f.a, &f.t);
        action::ResolveDamage(f.d);
        CHECK_FALSE(f.d.field.monsterZones[0][0].isEmpty());
        CHECK_FALSE(f.d.field.monsterZones[1][1].isEmpty());
        CHECK(f.d.lp[0] == DuelConfig::START_LP);
    }
}

// ── Direct attack + LP-depletion win (p.34, p.44) ────────────────────────────

TEST_CASE("Direct attack deals full ATK and can win the duel (p.34, p.44)",
          "[engine][battle][win]") {
    SECTION("full damage, no destruction") {
        BattleFix f{1000};
        f.d.field.monsterZones[1][1].remove(); // opponent field now empty
        REQUIRE(action::CanAttack(f.d, &f.a));
        REQUIRE(action::DeclareAttack(f.d, &f.a, nullptr).find("attacks") !=
                std::string::npos);
        std::string log = action::ResolveDamage(f.d);
        CHECK(log.find("directly") != std::string::npos);
        CHECK(f.d.lp[1] == DuelConfig::START_LP - 1000);
        CHECK(f.d.lp[0] == DuelConfig::START_LP);
        CHECK(f.d.result == DuelResult::Ongoing);
        // One attack per monster per turn: a second declaration is refused.
        REQUIRE(action::DeclareAttack(f.d, &f.a, nullptr).find("attack not possible") !=
                std::string::npos);
    }
    SECTION("LP reaching 0 ends the duel (p.44)") {
        BattleFix f{1000};
        f.d.field.monsterZones[1][1].remove();
        f.d.lp[1] = 500; // lethal
        action::DeclareAttack(f.d, &f.a, nullptr);
        action::ResolveDamage(f.d);
        CHECK(f.d.lp[1] <= 0);
        CHECK(f.d.result == DuelResult::Player0Win);
        CHECK(f.d.winReason == WinReason::LPDepletion);
    }
}

// ── Chains: Spell Speed rule + real LP effects (p.41) ────────────────────────


