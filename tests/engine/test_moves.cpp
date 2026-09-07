#include "support/Fixtures.hpp"

TEST_CASE("MoveDraw shifts the deck top card to the owner's hand", "[effect]") {
    Field f;
    Card a{}, b{};
    a.id = 1;
    a.state.owner = 0;
    a.state.controller = 0;
    b.id = 2;
    b.state.owner = 0;
    b.state.controller = 0;
    f.deckZones[0].put(&a);
    f.deckZones[0].put(&b);  // b is on top

    REQUIRE(MoveDraw(f, 0, 1) == 1);
    REQUIRE(f.deckZones[0].count() == 1);
    REQUIRE(f.handZones[0].count() == 1);
    REQUIRE(f.handZones[0].peek(-1) == &b);
}

TEST_CASE("MoveMillToGY sends the deck top card to the Graveyard", "[effect]") {
    Field f;
    Card a{}, b{};
    a.id = 1;
    a.state.owner = 0;
    a.state.controller = 0;
    b.id = 2;
    b.state.owner = 0;
    b.state.controller = 0;
    f.deckZones[0].put(&a);
    f.deckZones[0].put(&b);  // b on top

    REQUIRE(MoveMillToGY(f, 0, 1) == 1);
    REQUIRE(f.deckZones[0].count() == 1);
    REQUIRE(f.graveyardZones[0].count() == 1);
    REQUIRE(f.graveyardZones[0].peek(-1) == &b);
}

TEST_CASE("MoveDestroyToGY resolves to the controller's Graveyard (not owner's)",
          "[effect]") {
    Field f;
    Card m{};
    m.id = 42;
    m.state.owner = 0;
    m.state.controller = 1;  // owned by P0, controlled by P1
    m.attributes.push_back(Attribute::Monster);
    REQUIRE(f.monsterZones[1][0].put(&m));

    REQUIRE(MoveDestroyToGY(f, &m));
    REQUIRE(f.graveyardZones[1].count() == 1);  // controller's GY
    REQUIRE(f.graveyardZones[0].count() == 0);  // NOT owner's
    REQUIRE(f.monsterZones[1][0].isEmpty());
}

TEST_CASE("MoveBanish (face-down) hides the card but stays countable+findable",
          "[effect]") {
    Field f;
    Card m{};
    m.id = 7;
    m.state.owner = 0;
    m.state.controller = 0;
    m.attributes.push_back(Attribute::Monster);
    REQUIRE(f.monsterZones[0][0].put(&m));

    REQUIRE(MoveBanish(f, &m, /*faceDown=*/true));
    REQUIRE(f.monsterZones[0][0].isEmpty());
    REQUIRE(f.banishedZones[0].count() == 1);  // face-down cards count too
    REQUIRE(f.banishedZones[0].contains(&m));  // findable despite hidden
}

TEST_CASE("MoveReturnHand pulls a card back from the Graveyard", "[effect]") {
    Field f;
    Card m{};
    m.id = 9;
    m.state.owner = 1;
    m.state.controller = 1;
    f.graveyardZones[1].put(&m);

    REQUIRE(MoveReturnHand(f, &m));
    REQUIRE(f.graveyardZones[1].count() == 0);
    REQUIRE(f.handZones[1].count() == 1);
    REQUIRE(f.handZones[1].peek(-1) == &m);
}

TEST_CASE("Summon_Normal places a card face-up in an empty Monster Zone", "[effect]") {
    Field f;
    Card m{};
    m.id = 3000;
    m.state.owner = 0;
    m.state.controller = 0;
    m.attributes.push_back(Attribute::Monster);
    f.handZones[0].put(&m);  // normal summon from the hand

    REQUIRE(SummonToMMZ(f, &m, /*toPlayer=*/0, /*faceDown=*/false));
    REQUIRE(f.handZones[0].count() == 0);
    REQUIRE(f.monsterZones[0][0].peek() == &m);
    REQUIRE(f.monsterZones[0][0].isVertical());
    REQUIRE(f.monsterZones[0][0].isVisible());
}

// ── act dispatch ─────────────────────────────────────────────────
TEST_CASE("act dispatches each ActionId to its zone move", "[effect][resolver]") {
    Field f;
    Card d{};
    d.id = 1;
    d.state.owner = 0;
    d.state.controller = 0;
    f.deckZones[0].put(&d);

    std::string msg = action::MoveDraw(f, 0, 1) ? "1 card(s) drawn." : "draw failed.";

    REQUIRE(f.handZones[0].count() == 1);
    REQUIRE(msg.find("drawn") != std::string::npos);
}

// ── Engine: turn structure + chains (Rulebook p.30 / p.41) ───────────────────
