#include "support/Fixtures.hpp"

// ── from tests.cpp lines 380,394 ──
TEST_CASE("Duel owns Field + Life Points + turn + chain (layer 4)", "[duel]") {
    Duel d;
    REQUIRE(d.lp[0] == 8000);
    REQUIRE(d.lp[1] == 8000);
    REQUIRE(d.turn.phase == Phase::Draw);
    REQUIRE_FALSE(d.canAct());

    Card m{}; m.id = 1; m.state.owner = 0; m.state.controller = 0; m.attributes.push_back(Attribute::Monster);
    d.field.monsterZones[0][0].put(&m);
    auto found = d.field.findCard(&m);
    REQUIRE(found.first != nullptr);
    REQUIRE(found.first == &d.field.monsterZones[0][0]);
    REQUIRE(d.field.findCard(const_cast<const Card *>(&m)).first != nullptr);
}


// ── from tests.cpp lines 725,748 ──
TEST_CASE("Deck pointer seal: backing recorded, re-sealable, mismatch detectable",
          "[engine][duel]") {
    Duel d;
    Engine e(d);
    Card deck[3];
    std::vector<Card *> dv;
    for (int i = 0; i < 3; ++i)
        dv.push_back(mkMon(&deck[i], 800 + i, 4, 1000, 800));

    e.setDeck(0, dv); // seals the pointer projection's address
    e.sealDeckBacking(0, &dv); // app re-seals onto the OWNING vector
    REQUIRE(e.deckBackingMatches(0, &dv));
    REQUIRE(e.deckBackingMatches(1, nullptr) == false); // player 1 never sealed

    // A copy of the pointer vector lives elsewhere -> the seal detects it.
    std::vector<Card *> copy = dv;
    REQUIRE_FALSE(e.deckBackingMatches(0, &copy));
    // Re-sealing (e.g. rematch re-seat) updates the record, not duplicates it.
    e.sealDeckBacking(0, &copy);
    REQUIRE(e.deckBackingMatches(0, &copy));
    REQUIRE_FALSE(e.deckBackingMatches(0, &dv));
    REQUIRE(d.deckBackings.size() == 1); // still one backing per player
}


