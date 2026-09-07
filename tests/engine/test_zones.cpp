#include "support/Fixtures.hpp"

TEST_CASE("Single-slot Zone put / remove / contains / guards", "[zone]") {
    Zone_Monster slot;  // concrete single-slot zone (Zone/IZone is abstract)
    Card c;
    c.id = 42;

    REQUIRE(slot.isEmpty());
    REQUIRE(slot.count() == 0);

    REQUIRE(slot.put(&c));  // occupy
    REQUIRE(!slot.isEmpty());
    REQUIRE(slot.count() == 1);
    REQUIRE(slot.contains(&c));
    REQUIRE(slot.peek() == &c);

    REQUIRE(!slot.put(&c));  // already occupied
    REQUIRE(slot.remove(nullptr) == &c);
    REQUIRE(slot.isEmpty());

    REQUIRE(!slot.put(nullptr));               // cannot put null
    REQUIRE(slot.remove(nullptr) == nullptr);  // nothing to remove
}

TEST_CASE("ZoneStack push/peek/index/count semantics", "[zone]") {
    ZoneStack_Deck deck;
    Card a, b, c;
    a.id = 1;
    b.id = 2;
    c.id = 3;

    deck.put(&a);  // bottom
    deck.put(&b);
    deck.put(&c);  // top (back)

    REQUIRE(deck.count() == 3);
    REQUIRE(deck.peek(-1) == &c);  // top
    REQUIRE(deck.peek(0) == &a);   // bottom
    REQUIRE(deck.peek(2) == &c);

    deck.reset();
    REQUIRE(deck.count() == 0);
    REQUIRE(deck.isEmpty());
}

TEST_CASE("IZone::moveTo transfers the top card and rolls back on failure", "[zone]") {
    ZoneStack_Deck deck;
    ZoneStack_Graveyard gy;
    Zone_Monster ms;  // single-slot monster zone (only 1 card)
    Card a, b;
    a.id = 1;
    b.id = 2;

    deck.put(&a);
    deck.put(&b);  // deck top = b
    REQUIRE(deck.moveTo(gy));
    REQUIRE(deck.count() == 1);
    REQUIRE(gy.count() == 1);
    REQUIRE(gy.peek(-1) == &b);

    // Move deck top (a) to an already-occupied single slot -> must roll back.
    ms.put(&a);                      // ms now holds a
    REQUIRE_FALSE(deck.moveTo(ms));  // ms full -> rollback
    REQUIRE(deck.count() == 1);      // deck unchanged: still holds a
    REQUIRE(deck.peek(-1) == &a);    // top is a
    REQUIRE(ms.peek() == &a);        // ms unchanged
}

// --- Field ---

TEST_CASE("Field helper queries and clearField", "[field]") {
    Field f;
    Card m;

    SECTION("initially all monster zones empty for P1") {
        REQUIRE(f.firstEmptyMonsterZone(1) == 0);
        REQUIRE(f.firstOccupiedMonsterZone(1) == -1);
        REQUIRE(f.countMonsters(1) == 0);
    }

    SECTION("occupying a monster zone updates queries") {
        REQUIRE(f.monsterZones[1][2].put(&m));
        REQUIRE(f.firstOccupiedMonsterZone(1) == 2);
        REQUIRE(f.firstEmptyMonsterZone(1) == 0);
        REQUIRE(f.countMonsters(1) == 1);
        REQUIRE(f.countMonsters(0) == 0);
    }

    SECTION("extra monster zones start empty") {
        REQUIRE(f.firstEmptyExtraMonsterZone() == 0);
        REQUIRE(f.firstEmptyMonsterZone(0) == 0);
    }

    SECTION("clearField resets monster + spell/trap + field zones") {
        Card m;
        m.id = 7;
        m.state.controller = 0;
        f.monsterZones[0][4].put(&m);
        f.spellTrapZones[0][0].put(new Card{});
        f.fieldZones[1].put(new Card{});
        f.clearField();
        REQUIRE(f.firstOccupiedMonsterZone(0) == -1);
        REQUIRE(f.monsterZones[0][4].isEmpty());
        REQUIRE(f.spellTrapZones[0][0].isEmpty());
    }
}
