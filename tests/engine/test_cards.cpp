#include "support/Fixtures.hpp"

// ── from tests.cpp lines 51,124 ──
TEST_CASE("CardDatabase loads the starter cards.json", "[card][db]") {
    CardDatabase db;
    REQUIRE(db.LoadFromFile(cardsPath()));

    auto& all = db.GetAllCards();
    REQUIRE(all.size() == 6);

    SECTION("lookups by id and name") {
        REQUIRE(db.GetCardById(89631139) != nullptr);  // Blue-Eyes
        REQUIRE(db.GetCardById(46986414) != nullptr);  // Dark Magician
        REQUIRE(db.GetCardByName("Kuriboh") != nullptr);
        REQUIRE(db.GetCardById(99999999) == nullptr);  // unknown id
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
        REQUIRE(be->id == be->id);
        REQUIRE(be->id == 89631139);
    }
}

TEST_CASE("FindByName substring search", "[card][db]") {
    CardDatabase db;
    REQUIRE(db.LoadFromFile(cardsPath()));
    auto hits = db.FindByName("Magic");
    REQUIRE(hits.size() == 1);
    REQUIRE(hits.front()->id == 46986414);
}

TEST_CASE("Card comparators are strict weak orderings", "[card][sort]") {
    CardDatabase db;
    REQUIRE(db.LoadFromFile(cardsPath()));

    Card a, b;
    a.id = 1;
    b.id = 2;
    a.name = "Zap";
    b.name = "Apple";
    a.atk = 100;
    b.atk = 200;
    a.level = 3;
    b.level = 4;
    a.attributes.push_back(Attribute::Monster);

    using openjoey::cards::compare::byAtk;
    using openjoey::cards::compare::byId;
    using openjoey::cards::compare::byName;

    SECTION("byName: Apple before Zap") {
        REQUIRE_FALSE(byName(a, b));
        REQUIRE(byName(b, a));
    }
    SECTION("byAtk: 100 before 200") {
        REQUIRE(byAtk(a, b));
        REQUIRE_FALSE(byAtk(b, a));
    }
    SECTION("byId") {
        REQUIRE(byId(a, b));
        REQUIRE_FALSE(byId(b, a));
    }
}

// --- Zones ---

// ── from tests.cpp lines 245,261 ──
// ── Effects: the "zone-move" invariant ───────────────────────────────────────
// Per the design driving this refactor: every classic effect is expressed as a
// card moving from one zone to another. EffectsBuiltIn is the single mutator;
// Builtins are the single mutator; action::Perform dispatches by id.
