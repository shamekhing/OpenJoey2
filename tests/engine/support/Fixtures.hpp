#pragma once
// ── Shared test fixtures (extracted from the old monolithic tests.cpp) ──────
#include "catch.hpp"
// Integration/unit tests for OpenJoey2 core (card DB + zone/field logic).
// These do NOT link raylib: CardDatabase, Card, and game::zone are header-only
// and raylib-free, so tests run fast without a GL context.
#include "cards/cards.hpp"
#include "action/ActionSpec.hpp"
#include "engine/field/Field.hpp"

#include "engine/field/zone/ZoneEnums.hpp"
#include "engine/field/zone/IZone.hpp"
#include "engine/field/zone/Zone.hpp"
#include "engine/field/zone/ZoneStack.hpp"
#include "engine/field/zone/Zones.hpp"
#include "engine/config/DuelConfig.hpp"
#include "engine/protocol/BattleProtocol.hpp"
#include "engine/protocol/ChainProtocol.hpp"
#include "engine/duel/Chain.hpp"
#include "engine/duel/Duel.hpp"
#include "engine/duel/Engine.hpp"
#include "engine/action/Observe.hpp"
#include "engine/action/Perform.hpp"
#include "engine/protocol/DuelProtocol.hpp"
#include "engine/action/Catalog.hpp"
#include "Config.hpp"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

using namespace openjoey;
using namespace openjoey::engine;
using namespace openjoey::engine::zone;
using namespace openjoey::cards;
using namespace openjoey::engine::action;
using namespace openjoey::engine::action;

// Tests run against a deterministic 6-card fixture committed next to this file,
// NOT the full 14k-card database in openjoey-content. Resolve it relative to
// this source file so it works regardless of CTest's working directory.
inline std::string cardsPath() {
    std::filesystem::path p(__FILE__);          // tests/support/Fixtures.hpp
    return (p.parent_path().parent_path() / "fixtures" / "cards.json").string();
}

// --- Card database (validates the real CardParser + the shipped cards.json) ---
namespace {

// Fill out a bare monster card with deterministic identity/stats.
Card *mkMon(Card *m, uint32_t id, int lvl, int atk, int def) {
    m->id = id;
    m->name = "M" + std::to_string(id);
    m->attributes.push_back(Attribute::Monster);
    m->level = lvl;
    m->atk = atk;
    m->def = def;
    m->state.owner = 0;
    m->state.controller = 0;
    return m;
}

// Place a face-up ATK monster directly into a player's monster zone.
void fieldMonster(Duel &d, Card *m, int player, int slot) {
    m->state.owner = player;
    m->state.controller = player;
    d.field.monsterZones[player][slot].put(m);
    d.field.monsterZones[player][slot].changeVisibility(Visibility::Visible);
    d.field.monsterZones[player][slot].changeOrientation(Orientation::Vertical);
}

} // namespace
namespace {

// Standard battle setup: P0's turn, Battle Phase open, one monster each side.
struct BattleFix {
    Card a, t;
    Duel d;

    BattleFix(int aAtk, int tAtk = 0, int tDef = 0) {
        mkMon(&a, 9001, 4, aAtk, 1000);
        mkMon(&t, 9002, 4, tAtk, tDef);
        fieldMonster(d, &a, 0, 0);
        fieldMonster(d, &t, 1, 1);
        d.turnPlayer = 0;
        d.turn.turnNumber = 2;
        d.turn.phase = Phase::Battle;
        d.turn.skipBattle = false;
    }
};

} // namespace

