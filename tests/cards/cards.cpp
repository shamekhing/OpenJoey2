// openjoey-cards unit tests: CardDatabase, CardParser, comparators, ActionSpec.
// Raylib-free by design: links openjoey::cards + openjoey::foundation only.
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "cards/cards.hpp"
#include "action/ActionSpec.hpp"

#include <filesystem>
#include <string>
#include <type_traits>

using namespace openjoey;
using namespace openjoey::cards;
using namespace openjoey::cards;

// Tests run against a deterministic 6-card fixture committed next to this file,
// NOT the full 14k-card database in openjoey-content. Resolve it relative to
// this source file so it works regardless of CTest's working directory.
static std::string cardsPath() {
    std::filesystem::path p(__FILE__);
    return (p.parent_path() / "cards.json").string();
}

// --- CardDatabase ------------------------------------------------------------

TEST_CASE("CardDatabase loads the starter cards.json", "[db]") {
    CardDatabase db;
    REQUIRE(db.LoadFromFile(cardsPath()));
    REQUIRE(db.size() == 6);
    REQUIRE_FALSE(db.empty());

    SECTION("lookups by id and name") {
        REQUIRE(db.GetCardById(89631139) != nullptr);          // Blue-Eyes
        REQUIRE(db.GetCardById(46986414) != nullptr);          // Dark Magician
        REQUIRE(db.GetCardByName("Kuriboh") != nullptr);
        REQUIRE(db.GetCardById(99999999) == nullptr);          // unknown id
        REQUIRE(db.GetCardByName("No Such Card") == nullptr);  // unknown name
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

        // id mirrors the remote provider id (CardParser sets it)
        REQUIRE(be->id == 89631139);
        REQUIRE(be->hasAttribute(Attribute::Normal));

        // frame → attribute mapping (drives Extra-Deck
        // routing and other subtype behavior downstream).
        REQUIRE_FALSE(be->hasAttribute(Attribute::Effect));
        REQUIRE(mf->hasAttribute(Attribute::Trap));
        REQUIRE(ra->hasAttribute(Attribute::Spell));
    }
}

TEST_CASE("FindByName substring search is deterministic", "[db]") {
    CardDatabase db;
    REQUIRE(db.LoadFromFile(cardsPath()));

    auto hits = db.FindByName("Magic");
    REQUIRE(hits.size() == 1);
    REQUIRE(hits.front()->id == 46986414);

    // Results are sorted by id ascending regardless of hash order.
    auto all = db.FindByName(""); // empty needle matches every card
    REQUIRE(all.size() == db.size());
    for (std::size_t i = 1; i < all.size(); ++i)
        REQUIRE(all[i - 1]->id < all[i]->id);
}

TEST_CASE("LoadFromString parses an inline remote card-data payload", "[db][parser]") {
    CardDatabase db;
    REQUIRE(db.LoadFromString(R"({"data":[
        {"id":111,"name":"Alpha","desc":"","frameType":"normal","atk":100,"def":50,"level":2},
        {"id":222,"name":"Beta","desc":"","frameType":"spell"},
        {"id":222,"name":"Beta dupe","desc":"","frameType":"spell"},
        {"name":"NoId","desc":"","frameType":"trap"},
        {"id":"not-a-number"}
    ]})"));

    REQUIRE(db.size() == 2);
    REQUIRE(db.GetCardById(111)->atk == 100);
    REQUIRE(db.GetCardById(111)->isMonster());
    REQUIRE(db.GetCardById(222)->isSpell());
    REQUIRE(db.GetCardByName("Beta")->name == "Beta");       // duplicate id: first wins
    REQUIRE(db.GetCardByName("Beta dupe") == nullptr);
    REQUIRE(db.GetCardByName("NoId") == nullptr);            // id-less entry dropped
}

TEST_CASE("LoadFromFile missing file leaves the db empty", "[db]") {
    CardDatabase db;
    REQUIRE_FALSE(db.LoadFromFile("/nonexistent/path/cards.json"));
    REQUIRE(db.empty());
    REQUIRE(db.GetCardById(89631139) == nullptr);
}

TEST_CASE("Clear empties the database", "[db]") {
    CardDatabase db;
    REQUIRE(db.LoadFromFile(cardsPath()));
    db.Clear();
    REQUIRE(db.empty());
    REQUIRE(db.size() == 0);
    REQUIRE(db.GetCardById(89631139) == nullptr);
}

TEST_CASE("Database is movable but not copyable", "[db]") {
    STATIC_REQUIRE_FALSE(std::is_copy_constructible<CardDatabase>::value);
    STATIC_REQUIRE(std::is_move_constructible<CardDatabase>::value);

    CardDatabase source;
    REQUIRE(source.LoadFromFile(cardsPath()));
    const Card* be = source.GetCardById(89631139); // pointer into source storage

    CardDatabase moved(std::move(source));
    REQUIRE(moved.size() == 6);
    REQUIRE(moved.GetCardById(89631139) == be); // vector move keeps element addresses
}

// --- Card identity & presentation (Card.hpp) ---------------------------------

TEST_CASE("Card equality is identity-by-id", "[card]") {
    Card a, b;
    a.id = 7;
    b.id = 7;
    REQUIRE(a == b);
    REQUIRE_FALSE(a != b);

    a.name = "A";
    b.name = "B"; // different definitions, same id: still equal
    REQUIRE(a == b);

    Card zero; // id == 0: equals nothing, not even itself
    REQUIRE(zero != zero);
    REQUIRE_FALSE(zero == zero);
    REQUIRE(a != zero);
    REQUIRE(zero != a);
}

TEST_CASE("Card presentation helpers", "[card]") {
    Card be;
    be.id = 89631139;
    be.name   = "Blue-Eyes White Dragon";
    be.attributes.push_back(Attribute::Monster);
    be.atk    = 3000;
    be.def    = 2500;
    be.level  = 8;

    REQUIRE(be.cardTypeTag() == "[MON]");
    REQUIRE(be.statLine() == "Level 8  ATK 3000  DEF 2500");
    REQUIRE(be.shortStat() == "L8 3000/2500");
    REQUIRE_FALSE(be.isExtraDeckMonster());

    be.attributes.push_back(Attribute::Fusion);
    REQUIRE(be.isExtraDeckMonster());
    be.attributes.back() = Attribute::Synchro;
    REQUIRE(be.isExtraDeckMonster());
    be.attributes.back() = Attribute::Xyz;
    REQUIRE(be.isExtraDeckMonster());
    be.attributes.back() = Attribute::Normal;
    REQUIRE_FALSE(be.isExtraDeckMonster());

    Card spl;
    spl.attributes.push_back(Attribute::Spell);
    REQUIRE(spl.cardTypeTag() == "[SPL]");
    REQUIRE(spl.statLine().empty());
    REQUIRE(spl.shortStat().empty());

    Card trp;
    trp.attributes.push_back(Attribute::Trap);
    REQUIRE(trp.cardTypeTag() == "[TRP]");
}

// --- CardDatabase extras ------------------------------------------------------

TEST_CASE("FindByName returns every card whose name matches", "[db]") {
    CardDatabase db;
    REQUIRE(db.LoadFromString(R"({"data":[
        {"id":1,"name":"Dup","desc":"","frameType":"normal"},
        {"id":2,"name":"Dup","desc":"","frameType":"effect"},
        {"id":3,"name":"Other","desc":"","frameType":"spell"}
    ]})"));

    auto hits = db.FindByName("Dup");
    REQUIRE(hits.size() == 2);
    REQUIRE(hits[0]->id == 1);
    REQUIRE(hits[1]->id == 2);

    // Exact lookup still resolves the first card with that name.
    REQUIRE(db.GetCardByName("Dup")->id == 1);
    REQUIRE(db.FindByName("").size() == db.size()); // empty needle matches all
}

TEST_CASE("GetAllCards is read-only and the index stays coherent", "[db]") {
    CardDatabase db;
    REQUIRE(db.LoadFromFile(cardsPath()));

    const CardDatabase &view = db;
    STATIC_REQUIRE(
        std::is_same<decltype(view.GetAllCards()), const std::vector<Card> &>::value);
    REQUIRE(view.GetAllCards().size() == db.size());

    // Mutating gameplay fields through the lookup API must not dangle the
    // index. (Identity fields — name, id — are snapshotted into the name
    // index at load time and must NOT be mutated through the pointer.)
    db.GetCardById(89631139)->atk = 9999;
    REQUIRE(db.GetCardById(89631139)->atk == 9999);
    REQUIRE(db.GetCardByName("Blue-Eyes White Dragon") != nullptr);
    REQUIRE(db.GetCardByName("Blue-Eyes White Dragon")->atk == 9999);
}

// --- CardParser --------------------------------------------------------------

TEST_CASE("Parser rejects malformed payloads without throwing", "[parser]") {
    REQUIRE_FALSE(cards::parseRemoteCardJson("").ok());
    REQUIRE_FALSE(cards::parseRemoteCardJson("not json at all").ok());
    REQUIRE_FALSE(cards::parseRemoteCardJson(R"({"data": 42})").ok());
    REQUIRE_FALSE(cards::parseRemoteCardJson(R"([1,2,3])").ok());
}

TEST_CASE("Parser maps stat edge cases to sane values", "[parser]") {
    SECTION("'?' and string stats parse to 0") {
        auto r = cards::parseRemoteCardJson(
            R"({"data":[{"id":333,"name":"Mystic","frameType":"normal","atk":"?","def":"?","level":4}]})");
        REQUIRE(r.ok());
        REQUIRE(r.errors.empty());
        REQUIRE(r.cards.size() == 1);
        REQUIRE(r.cards[0].atk == 0);
        REQUIRE(r.cards[0].def == 0);
        REQUIRE(r.cards[0].level == 4);
    }
    SECTION("missing name is synthesized from the id") {
        auto r = cards::parseRemoteCardJson(R"({"data":[{"id":444,"frameType":"trap"}]})");
        REQUIRE(r.ok());
        REQUIRE(r.cards[0].name == "Card 444");
        REQUIRE(r.cards[0].isTrap());
    }
    SECTION("rank falls back into level for Xyz frames") {
        auto r = cards::parseRemoteCardJson(
            R"({"data":[{"id":555,"name":"Xyz","frameType":"xyz","rank":5}]})");
        REQUIRE(r.ok());
        REQUIRE(r.cards[0].level == 5);
    }
    SECTION("non-object entries are skipped, not fatal") {
        auto r = cards::parseRemoteCardJson(
            R"({"data":[42,{"id":666,"name":"Ok","frameType":"spell"}]})");
        REQUIRE(r.ok());
        REQUIRE(r.cards.size() == 1);
        REQUIRE_FALSE(r.errors.empty());
    }
}

// --- Comparators (Compare.hpp) -----------------------------------------------

TEST_CASE("Comparators are strict weak orderings", "[compare]") {
    Card a, b;
    a.id = 1; b.id = 2;
    a.name = "Zap"; b.name = "Apple";
    a.atk = 100; b.atk = 200;
    a.level = 3; b.level = 4;
    a.attributes.push_back(Attribute::Monster);

    using openjoey::cards::compare::byAtk;
    using openjoey::cards::compare::byDef;
    using openjoey::cards::compare::byId;
    using openjoey::cards::compare::byLevel;
    using openjoey::cards::compare::byName;
    using openjoey::cards::compare::byFrame;

    SECTION("byName: Apple before Zap") {
        REQUIRE_FALSE(byName(a, b));
        REQUIRE(byName(b, a));
    }
    SECTION("byAtk: 100 before 200") {
        REQUIRE(byAtk(a, b));
        REQUIRE_FALSE(byAtk(b, a));
    }
    SECTION("byLevel") {
        REQUIRE(byLevel(a, b));
        REQUIRE_FALSE(byLevel(b, a));
    }
    SECTION("byId") {
        REQUIRE(byId(a, b));
        REQUIRE_FALSE(byId(b, a));
    }

    SECTION("byFrame: Monster < Spell < Trap, name tiebreak") {
        Card m, s, t;
        m.id = 10;
        s.id = 11;
        t.id = 12;
        m.name = "Same";
        s.name = "Same";
        t.name = "Same";
        m.attributes.push_back(Attribute::Monster);
        s.attributes.push_back(Attribute::Spell);
        t.attributes.push_back(Attribute::Trap);
        REQUIRE(byFrame(m, s));
        REQUIRE(byFrame(s, t));
        REQUIRE_FALSE(byFrame(t, s));
    }

    SECTION("byDef: 100 before 200") {
        a.def = 100;
        b.def = 200;
        REQUIRE(byDef(a, b));
        REQUIRE_FALSE(byDef(b, a));
    }
}

// --- ActionSpec (brace-init contract) ----------------------------------------

TEST_CASE("ActionSpec aggregate field order is stable", "[effect]") {
    // openjoey-gameplay brace-initializes ActionSpec positionally; this test
    // pins the field order as an explicit API contract.
    ActionSpec e{ActionId::Move_Draw, EffectType::Trigger, 2, 100, 0,
                 TargetScope::None, false, "draw 100"};
    REQUIRE(e.id == ActionId::Move_Draw);
    REQUIRE(e.timing == EffectType::Trigger);
    REQUIRE(e.speed == 2);
    REQUIRE(e.amount == 100);
    REQUIRE(e.lpCost == 0);
    REQUIRE(e.scope == TargetScope::None);
    REQUIRE_FALSE(e.needsTarget);
    REQUIRE(std::string(e.note) == "draw 100");

    ActionSpec def{}; // defaults: None / Ignition / speed 1 / amount 1 / no cost
    REQUIRE(def.id == ActionId::None);
    REQUIRE(def.timing == EffectType::Ignition);
    REQUIRE(def.speed == 1);
    REQUIRE(def.amount == 1);
    REQUIRE(def.lpCost == 0);
    REQUIRE(def.scope == TargetScope::None);
}

