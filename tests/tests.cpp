// Integration/unit tests for OpenJoey2 core (card DB + zone/field logic).
// These do NOT link raylib: CardDatabase, Card, and game::zone are header-only
// and raylib-free, so tests run fast without a GL context.
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "card/Card.hpp"
#include "card/CardDatabase.hpp"
#include "game/zone/Field.hpp"
#include "game/zone/Zone.hpp"
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

        // imageId mirrors cardNumber (CardParser sets it)
        REQUIRE(be->imageId == be->cardNumber);
        REQUIRE(be->imageId == 89631139);
    }
}

TEST_CASE("GetCardsByName substring search", "[card][db]") {
    CardDatabase db;
    REQUIRE(db.LoadFromFile(cardsPath()));
    auto hits = db.GetCardsByName("Magic");
    REQUIRE(hits.size() == 1);
    REQUIRE(hits.front()->cardNumber == 46986414);
}

TEST_CASE("Card static comparators are strict weak orderings", "[card][sort]") {
    CardDatabase db;
    REQUIRE(db.LoadFromFile(cardsPath()));

    Card a, b;
    a.cardNumber = 1; b.cardNumber = 2;
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
    Card c; c.cardNumber = 42;

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
    a.cardNumber = 1; b.cardNumber = 2; c.cardNumber = 3;

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
    a.cardNumber = 1; b.cardNumber = 2;

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
        Card m; m.cardNumber = 1; m.controller = 1; m.position = Position::FaceUp;
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
        Card m; m.cardNumber = 7; m.controller = 0;
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
    std::filesystem::remove(ContentPaths::userSettingsJson(), ec);
}
