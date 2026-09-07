#include "support/Fixtures.hpp"

// ── from tests.cpp lines 1270,1357 ──
TEST_CASE("classicEffectsFor maps wired classic cards and skips the rest",
          "[classic][catalog]") {
    CHECK(classicEffectsFor("Pot of Greed").size() == 1);
    CHECK(classicEffectsFor("Pot of Greed")[0].id == ActionId::Move_Draw);
    CHECK(classicEffectsFor("Pot of Greed")[0].amount == 2);
    CHECK(classicEffectsFor("Graceful Charity").size() == 2);
    CHECK(classicEffectsFor("Solemn Judgment")[0].speed == 3);
    CHECK(classicEffectsFor("Solemn Judgment")[0].lpCost == 2000);
    CHECK(classicEffectsFor("Man-Eater Bug")[0].timing ==
          EffectType::Trigger);
    CHECK(classicEffectsFor("Raigeki")[0].scope == TargetScope::OppMonsters);
    CHECK(classicEffectsFor("Dark Hole")[0].scope == TargetScope::AllMonsters);
    CHECK(classicEffectsFor("Heavy Storm")[0].scope == TargetScope::AllSpellsTraps);
    // Normal monsters have no wired effects.
    CHECK(classicEffectsFor("Gemini Elf").empty());
    CHECK(classicEffectsFor("Blue-Eyes White Dragon").empty());
    CHECK(classicEffectsFor("Not A Real Card").empty());
}

TEST_CASE("Catalog scopes: damage targets the opponent, gains self",
          "[classic][catalog]") {
    // Scope replaces the old classicChainArgs sentinel protocol.
    CHECK(classicEffectsFor("Ookazi")[0].scope == TargetScope::Opponent);
    CHECK(classicEffectsFor("Ookazi")[0].amount == 800);
    CHECK(classicEffectsFor("Dian Keto the Cure Master")[0].scope ==
          TargetScope::Activator);
    CHECK(classicEffectsFor("Dian Keto the Cure Master")[0].amount == 1000);
    // Graceful Charity: draw 3 (self) then Cost-discard (self).
    const auto charity = classicEffectsFor("Graceful Charity");
    CHECK(charity.size() == 2);
    CHECK(charity[0].amount == 3);
    CHECK(charity[1].scope == TargetScope::Activator);
    // Delinquent Duo: pay 1000 (self cost), then discard 1 from the OPPONENT's
    // hand — two specs, like the card's text.
    const auto duo = classicEffectsFor("Delinquent Duo");
    CHECK(duo.size() == 2);
    CHECK(duo[0].lpCost == 1000);
    CHECK(duo[0].scope == TargetScope::Activator);
    CHECK(duo[1].scope == TargetScope::Opponent);
    // Raigeki: mass mode, no specific target.
    const auto raigeki = classicEffectsFor("Raigeki")[0];
    CHECK(raigeki.scope == TargetScope::OppMonsters);
    CHECK(raigeki.needsTarget == false);
}

TEST_CASE("Mass destruction via the resolver: Raigeki / Dark Hole / Heavy Storm",
          "[classic][mass]") {
    SECTION("Raigeki clears the opponent's monsters only") {
        Duel d;
        Card a1, a2, b1;
        fieldMonster(d, mkMon(&a1, 801, 4, 1000, 800), 1, 0);
        fieldMonster(d, mkMon(&a2, 802, 4, 1200, 900), 1, 1);
        fieldMonster(d, mkMon(&b1, 803, 4, 1300, 700), 0, 0);
        int n = 0;
        for (auto &mz : d.field.monsterZones[1])
            if (Card *c = mz.peek()) {
                MoveDestroyToGY(d.field, c);
                ++n;
            }
        std::string msg = std::to_string(n) + " card(s) destroyed -> Graveyard.";
        CHECK(msg.find("2 card(s) destroyed") != std::string::npos);
        CHECK(d.field.monsterZones[1][0].isEmpty());
        CHECK(d.field.monsterZones[1][1].isEmpty());
        CHECK(d.field.monsterZones[0][0].contains(&b1));  // own monster spared
        CHECK(d.field.graveyardZones[1].contains(&a1));
    }
    SECTION("Dark Hole clears both sides") {
        Duel d;
        Card a1, b1;
        fieldMonster(d, mkMon(&a1, 811, 4, 1000, 800), 0, 0);
        fieldMonster(d, mkMon(&b1, 812, 4, 1200, 900), 1, 0);
        int nd = action::MoveDestroyMass(d.field, TargetScope::AllMonsters, 0);
        std::string msg = std::to_string(nd) + " card(s) destroyed -> Graveyard.";
        CHECK(msg.find("2 card(s) destroyed") != std::string::npos);
        CHECK(d.field.monsterZones[0][0].isEmpty());
        CHECK(d.field.monsterZones[1][0].isEmpty());
    }
    SECTION("Heavy Storm clears Spells/Traps only") {
        Duel d;
        Card st1, mon;
        st1.id = 900;
        st1.name = "Solemn";
        st1.attributes.push_back(Attribute::Trap);
        d.field.spellTrapZones[0][0].put(&st1);
        fieldMonster(d, mkMon(&mon, 901, 4, 1000, 800), 1, 0);
        int nd = action::MoveDestroyMass(d.field, TargetScope::AllSpellsTraps, 1);
        std::string msg = std::to_string(nd) + " card(s) destroyed -> Graveyard.";
        CHECK(msg.find("1 card(s) destroyed") != std::string::npos);
        CHECK(d.field.spellTrapZones[0][0].isEmpty());
        CHECK(d.field.monsterZones[1][0].contains(&mon));  // monster spared
    }
}
