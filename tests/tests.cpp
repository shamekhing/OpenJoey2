// Integration/unit tests for OpenJoey2 core (card DB + zone/field logic).
// These do NOT link raylib: CardDatabase, Card, and game::zone are header-only
// and raylib-free, so tests run fast without a GL context.
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "card/Card.hpp"
#include "card/CardDatabase.hpp"
#include "card/CardEffect.hpp"
#include "card/EffectID.hpp"
#include "field/Field.hpp"
#include "field/EffectResolver.hpp"
#include "field/EffectsBuiltIn.hpp"
#include "zone/Zone.hpp"
#include "duel/Chain.hpp"
#include "duel/Duel.hpp"
#include "duel/TurnManager.hpp"
#include "ui/platform/Settings.hpp"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

using namespace openjoey;
using namespace openjoey::zone;

// Resolve data/cards.json relative to this source file so the test works
// regardless of the working directory CTest launches from.
static std::string cardsPath() {
    std::filesystem::path p(__FILE__);
    return (p.parent_path().parent_path() / "data" / "cards.json").string();
}

// --- Card database (validates the real CardParser + the shipped cards.json) ---

TEST_CASE("CardDatabase loads the starter cards.json", "[card][db]") {
    CardDatabase db;
    REQUIRE(db.LoadFromFile(cardsPath()));

    auto& all = db.GetAllCards();
    REQUIRE(all.size() == 6);

    SECTION("lookups by id and name") {
        REQUIRE(db.GetCardById(89631139) != nullptr);          // Blue-Eyes
        REQUIRE(db.GetCardById(46986414) != nullptr);          // Dark Magician
        REQUIRE(db.GetCardByName("Kuriboh") != nullptr);
        REQUIRE(db.GetCardById(99999999) == nullptr);          // unknown id
    }

    SECTION("parsed fields match the JSON") {
        const Card* be = db.GetCardById(89631139);
        REQUIRE(be->name == "Blue-Eyes White Dragon");
        REQUIRE(be->isMonster());
        REQUIRE(be->atk == 3000);
        REQUIRE(be->def == 2500);
        REQUIRE(be->level == 8);

        const Card* ra = db.GetCardById(12580477);
        REQUIRE(ra->isSpell());
        REQUIRE(ra->atk == 0);

        const Card* mf = db.GetCardById(44095762);
        REQUIRE(mf->isTrap());

        // imageId mirrors cardId (CardParser sets it)
        REQUIRE(be->imageId == be->cardId);
        REQUIRE(be->imageId == 89631139);
    }
}

TEST_CASE("GetCardsByName substring search", "[card][db]") {
    CardDatabase db;
    REQUIRE(db.LoadFromFile(cardsPath()));
    auto hits = db.GetCardsByName("Magic");
    REQUIRE(hits.size() == 1);
    REQUIRE(hits.front()->cardId == 46986414);
}

TEST_CASE("Card static comparators are strict weak orderings", "[card][sort]") {
    CardDatabase db;
    REQUIRE(db.LoadFromFile(cardsPath()));

    Card a, b;
    a.cardId = 1; b.cardId = 2;
    a.name = "Zap"; b.name = "Apple";
    a.atk = 100; b.atk = 200;
    a.level = 3; b.level = 4;
    a.type = CardType::Monster;

    SECTION("sortByName: Apple before Zap") {
        REQUIRE_FALSE(Card::sortByName(a, b));
        REQUIRE(Card::sortByName(b, a));
    }
    SECTION("sortByAtk: 100 before 200") {
        REQUIRE(Card::sortByAtk(a, b));
        REQUIRE_FALSE(Card::sortByAtk(b, a));
    }
    SECTION("sortById") {
        REQUIRE(Card::sortById(a, b));
        REQUIRE_FALSE(Card::sortById(b, a));
    }
}

// --- Zones ---

TEST_CASE("Single-slot Zone put / remove / contains / guards", "[zone]") {
    Zone_Monster slot;   // concrete single-slot zone (Zone/IZone is abstract)
    Card c; c.cardId = 42;

    REQUIRE(slot.isEmpty());
    REQUIRE(slot.count() == 0);

    REQUIRE(slot.put(&c));            // occupy
    REQUIRE(!slot.isEmpty());
    REQUIRE(slot.count() == 1);
    REQUIRE(slot.contains(&c));
    REQUIRE(slot.peek() == &c);

    REQUIRE(!slot.put(&c));           // already occupied
    REQUIRE(slot.remove(nullptr) == &c);
    REQUIRE(slot.isEmpty());

    REQUIRE(!slot.put(nullptr));      // cannot put null
    REQUIRE(slot.remove(nullptr) == nullptr); // nothing to remove
}

TEST_CASE("ZoneStack push/peek/index/count semantics", "[zone]") {
    ZoneStack_Deck deck;
    Card a, b, c;
    a.cardId = 1; b.cardId = 2; c.cardId = 3;

    deck.put(&a);   // bottom
    deck.put(&b);
    deck.put(&c);   // top (back)

    REQUIRE(deck.count() == 3);
    REQUIRE(deck.peek(-1) == &c);     // top
    REQUIRE(deck.peek(0) == &a);      // bottom
    REQUIRE(deck.peek(2) == &c);

    deck.reset();
    REQUIRE(deck.count() == 0);
    REQUIRE(deck.isEmpty());
}

TEST_CASE("IZone::moveTo transfers the top card and rolls back on failure", "[zone]") {
    ZoneStack_Deck deck;
    ZoneStack_Graveyard gy;
    Zone_Monster ms;                  // single-slot monster zone (only 1 card)
    Card a, b;
    a.cardId = 1; b.cardId = 2;

    deck.put(&a); deck.put(&b);       // deck top = b
    REQUIRE(deck.moveTo(gy));
    REQUIRE(deck.count() == 1);
    REQUIRE(gy.count() == 1);
    REQUIRE(gy.peek(-1) == &b);

        // Move deck top (a) to an already-occupied single slot -> must roll back.
    ms.put(&a);                       // ms now holds a
    REQUIRE_FALSE(deck.moveTo(ms));   // ms full -> rollback
    REQUIRE(deck.count() == 1);       // deck unchanged: still holds a
    REQUIRE(deck.peek(-1) == &a);     // top is a
    REQUIRE(ms.peek() == &a);         // ms unchanged
}

// --- Field ---

TEST_CASE("Field helper queries and clearField", "[field]") {
    Field f;

    SECTION("initially all monster zones empty for P1") {
        REQUIRE(f.firstEmptyMonsterZone(1) == 0);
        REQUIRE(f.firstOccupiedMonsterZone(1) == -1);
        REQUIRE(f.countMonsters(1) == 0);
    }

    SECTION("occupying a monster zone updates queries") {
        Card m; m.cardId = 1; m.controller = 1; m.position = Position::FaceUp;
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
        Card m; m.cardId = 7; m.controller = 0;
        f.monsterZones[0][4].put(&m);
        f.spellTrapZones[0][0].put(new Card{});
        f.fieldZones[1].put(new Card{});
        f.clearField();
        REQUIRE(f.firstOccupiedMonsterZone(0) == -1);
        REQUIRE(f.monsterZones[0][4].isEmpty());
        REQUIRE(f.spellTrapZones[0][0].isEmpty());
    }
}

TEST_CASE("Settings round-trips user_settings.json", "[settings]") {
    ui::Settings s;
    s.screenWidth = 1366;
    s.screenHeight = 768;
    s.targetFps = 120;
    s.fullscreen = true;
    s.downloadImages = false;
    REQUIRE(s.Save());

    ui::Settings loaded = ui::Settings::Load();
    REQUIRE(loaded.screenWidth == 1366);
    REQUIRE(loaded.screenHeight == 768);
    REQUIRE(loaded.targetFps == 120);
    REQUIRE(loaded.fullscreen == true);
    REQUIRE(loaded.downloadImages == false);

        std::error_code ec;
    std::filesystem::remove(ui::Settings::settingsFile(), ec);
}

// ── Card effect subscription (layer 1 carries the data; no later-layer dep) ──

TEST_CASE("A Card subscribes to EffectIDs it can activate", "[card][effect]") {
    Card c;
    c.name = "Test Subscriber";
    c.effects.push_back(CardEffect{EffectID::Move_Draw, EffectType::Ignition, 1});
    c.effects.push_back(CardEffect{EffectID::Move_DestroyToGY, EffectType::Trigger, 2});
    REQUIRE(c.effects.size() == 2);
    REQUIRE(c.effects[0].id == EffectID::Move_Draw);
    REQUIRE(c.effects[0].speed == 1);
    REQUIRE(c.effects[1].timing == EffectType::Trigger);
    REQUIRE(c.effects[1].speed == 2);
}

// ── Effects: the "zone-move" invariant ───────────────────────────────────────
// Per the design driving this refactor: every classic effect is expressed as a
// card moving from one zone to another. EffectsBuiltIn is the single mutator;
// EffectResolver dispatches by EffectID.

TEST_CASE("Move_Draw shifts the deck top card to the owner's hand", "[effect]") {
    Field f;
    Card a{}, b{};
    a.cardId = 1; a.owner = 0; a.controller = 0;
    b.cardId = 2; b.owner = 0; b.controller = 0;
    f.deckZones[0].put(&a); f.deckZones[0].put(&b); // b is on top

    REQUIRE(Move_Draw(f, 0, 1) == 1);
    REQUIRE(f.deckZones[0].count() == 1);
    REQUIRE(f.handZones[0].count() == 1);
    REQUIRE(f.handZones[0].peek(-1) == &b);
    REQUIRE(b.location == Location::Hand);
}

TEST_CASE("Move_MillToGY sends the deck top card to the Graveyard", "[effect]") {
    Field f;
    Card a{}, b{};
    a.cardId = 1; a.owner = 0; a.controller = 0;
    b.cardId = 2; b.owner = 0; b.controller = 0;
    f.deckZones[0].put(&a); f.deckZones[0].put(&b); // b on top

    REQUIRE(Move_MillToGY(f, 0, 1) == 1);
    REQUIRE(f.deckZones[0].count() == 1);
    REQUIRE(f.graveyardZones[0].count() == 1);
    REQUIRE(f.graveyardZones[0].peek(-1) == &b);
        REQUIRE(b.location == Location::Graveyard);
}

TEST_CASE("Move_DestroyToGY resolves to the controller's Graveyard (not owner's)",
          "[effect]") {
    Field f;
    Card m{};
    m.cardId = 42; m.owner = 0; m.controller = 1; // owned by P0, controlled by P1
    m.type = CardType::Monster;
    REQUIRE(f.monsterZones[1][0].put(&m));

    REQUIRE(Move_DestroyToGY(f, &m));
    REQUIRE(f.graveyardZones[1].count() == 1); // controller's GY
    REQUIRE(f.graveyardZones[0].count() == 0); // NOT owner's
    REQUIRE(f.monsterZones[1][0].isEmpty());
    REQUIRE(m.location == Location::Graveyard);
}

TEST_CASE("Move_Banish (face-down) hides the card but stays countable+findable",
          "[effect]") {
    Field f;
    Card m{};
    m.cardId = 7; m.owner = 0; m.controller = 0; m.type = CardType::Monster;
    REQUIRE(f.monsterZones[0][0].put(&m));

    REQUIRE(Move_Banish(f, &m, /*faceDown=*/true));
    REQUIRE(f.monsterZones[0][0].isEmpty());
    REQUIRE(f.banishedZones[0].count() == 1);          // face-down cards count too
    REQUIRE(f.banishedZones[0].contains(&m));         // findable despite hidden
    REQUIRE(m.location == Location::Banished);
}

TEST_CASE("Move_ReturnHand pulls a card back from the Graveyard", "[effect]") {
    Field f;
    Card m{};
    m.cardId = 9; m.owner = 1; m.controller = 1;
    f.graveyardZones[1].put(&m);

    REQUIRE(Move_ReturnHand(f, &m));
    REQUIRE(f.graveyardZones[1].count() == 0);
    REQUIRE(f.handZones[1].count() == 1);
    REQUIRE(f.handZones[1].peek(-1) == &m);
    REQUIRE(m.location == Location::Hand);
}

TEST_CASE("Summon_Normal places a card face-up in an empty Monster Zone", "[effect]") {
    Field f;
    Card m{};
    m.cardId = 3000; m.owner = 0; m.controller = 0; m.type = CardType::Monster;
    f.handZones[0].put(&m); // normal summon from the hand

    REQUIRE(Summon_Normal(f, &m, /*toPlayer=*/0));
    REQUIRE(f.handZones[0].count() == 0);
    REQUIRE(f.monsterZones[0][0].peek() == &m);
    REQUIRE(f.monsterZones[0][0].isVertical());
    REQUIRE(f.monsterZones[0][0].isVisible());
        REQUIRE(m.location == Location::Field);
}

// ── EffectResolver dispatch ─────────────────────────────────────────────────
TEST_CASE("EffectResolver dispatches each EffectID to its zone move", "[effect][resolver]") {
    Field f;
    Card d{}; d.cardId = 1; d.owner = 0; d.controller = 0;
    f.deckZones[0].put(&d);

    EffectResolver r(f);
    std::string msg = r.apply(EffectID::Move_Draw, /*activator=*/0);

    REQUIRE(f.handZones[0].count() == 1);
    REQUIRE(msg.find("drawn") != std::string::npos);
}

// ── Engine: turn structure + chains (Rulebook p.30 / p.41) ───────────────────
TEST_CASE("TurnManager walks Draw->Standby->Main1->Battle->Main2->End (p.30)",
          "[duel][turn]") {
    TurnManager t;
    REQUIRE(t.phase == Phase::Draw);
    REQUIRE_FALSE(t.canAct()); // Draw step is not an action window

    t.nextPhase(); REQUIRE(t.phase == Phase::Standby);
    t.nextPhase(); REQUIRE(t.phase == Phase::Main1);  REQUIRE(t.canAct());
    t.nextPhase(); REQUIRE(t.phase == Phase::Battle); REQUIRE(t.canAct());
    t.nextPhase(); REQUIRE(t.phase == Phase::Main2);  REQUIRE(t.canAct());
    t.nextPhase(); REQUIRE(t.phase == Phase::End);    REQUIRE_FALSE(t.canAct());
    t.nextPhase(); REQUIRE(t.phase == Phase::Draw);   REQUIRE(t.turnNumber == 2);
}

TEST_CASE("Chain resolves last-activated-first (p.41)", "[duel][chain]") {
    Chain c;
    c.push(EffectID::Move_Draw,   0, 1); // activated first
    c.push(EffectID::LP_Damage,  1, 2); // response
    c.push(EffectID::Move_Banish, 0, 3); // counter, fastest

    auto order = c.resolutionOrder();
    REQUIRE(order.size() == 3);
    REQUIRE(order[0]->id == EffectID::Move_Banish); // last activated, first resolved
    REQUIRE(order[1]->id == EffectID::LP_Damage);
    REQUIRE(order[2]->id == EffectID::Move_Draw);    // first activated, last resolved
}

TEST_CASE("Duel owns Field + Life Points + turn + chain (layer 4)", "[duel]") {
    Duel d;
    REQUIRE(d.lp[0] == 8000);
    REQUIRE(d.lp[1] == 8000);
    REQUIRE(d.turn.phase == Phase::Draw);
    REQUIRE_FALSE(d.canAct());

    Card m{}; m.cardId = 1; m.owner = 0; m.controller = 0; m.type = CardType::Monster;
    d.field.monsterZones[0][0].put(&m);
    auto found = d.field.findCard(&m);
    REQUIRE(found.first != nullptr);
    REQUIRE(found.first == &d.field.monsterZones[0][0]);
    REQUIRE(d.field.findCard(const_cast<const Card *>(&m)).first != nullptr);
}
