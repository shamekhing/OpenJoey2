#include "support/Fixtures.hpp"

// ── from tests.cpp lines 367,379 ──
TEST_CASE("Chain resolves last-activated-first (p.41)", "[duel][chain]") {
    Chain c;
    c.push(ActionSpec{ActionId::Move_Draw, EffectType::Ignition, 1}, 0); // activated first
    c.push(ActionSpec{ActionId::LP_Damage, EffectType::Ignition, 2}, 1); // response
    c.push(ActionSpec{ActionId::Move_Banish, EffectType::Quick, 3}, 0);  // counter, fastest

    auto order = c.resolutionOrder();
    REQUIRE(order.size() == 3);
    REQUIRE(order[0]->id == ActionId::Move_Banish); // last activated, first resolved
    REQUIRE(order[1]->id == ActionId::LP_Damage);
    REQUIRE(order[2]->id == ActionId::Move_Draw);    // first activated, last resolved
}


// ── from tests.cpp lines 596,629 ──
TEST_CASE("Trap set this turn cannot be activated; next turn it can (p.31)",
          "[engine][trap][chain]") {
    Duel d;
    Card trap;
    trap.id = 700;
    trap.name = "TRP700";
    trap.attributes.push_back(Attribute::Trap);
    // The spec is activated directly (catalog-independent — Card carries no
    // effects; real effects resolve by name through the classic catalog).
    ActionSpec trapFx{ActionId::LP_Damage, EffectType::Quick, 2, 500};
    trap.state.owner = trap.state.controller = 0;
    d.field.handZones[0].put(&trap);
    d.turnPlayer = 0;
    d.turn.turnNumber = 2;
    d.turn.phase = Phase::Main1;

    REQUIRE(action::SeatSpellTrap(d.field, &trap));
    REQUIRE(trap.state.setThisTurn);

    // Same turn: rejected (set-turn check keyed off args.source).
    ActionArgs same;
    same.source = &trap;
    REQUIRE(action::ActivateEffect(d, trapFx, 0, same)
                .find("turn it was Set") != std::string::npos);
    REQUIRE(d.chain.links.empty());

    // Next turn the flag has been cleared by the turn swap — activation works.
    REQUIRE(action::EndTurn(d).find("hand limit OK") != std::string::npos);
    ActionArgs next;
    next.source = &trap;
    REQUIRE(action::ActivateEffect(d, trapFx, 0, next)
                .find("Chain Link") != std::string::npos);
    action::ResolveChain(d);
}


// ── from tests.cpp lines 706,724 ──
TEST_CASE("PassResponse: disabled flag no-ops; ChainWaiting stays false (p.45 mode off)",
          "[engine][chain]") {
    Duel d;
    d.turnPlayer = 0;
    d.turn.turnNumber = 2;
    d.turn.phase = Phase::Main1;
    // Legacy mode: explicit resolution. PassResponse must not resolve or error.
    CHECK(action::PassResponse(d, 0).find("disabled") != std::string::npos);
    CHECK_FALSE(action::ChainWaiting(d));
    // An open chain + disabled flag: passes don't resolve it.
    ActionSpec dmg{ActionId::LP_Damage, EffectType::Ignition, 1, 100};
    CHECK(action::ActivateEffect(d, dmg, 0).find("Chain Link") != std::string::npos);
    CHECK(action::PassResponse(d, 0).find("disabled") != std::string::npos);
    CHECK(d.chain.links.size() == 1); // still open — resolved explicitly
    CHECK_FALSE(action::ChainWaiting(d));
    action::ResolveChain(d);
    CHECK(d.chain.links.empty());
}


// ── from tests.cpp lines 924,968 ──
TEST_CASE("Chain Spell Speed rule + LP effects mutate Life Points (p.41)",
          "[engine][chain]") {
    Duel d;

    // Link 1: any Spell Speed may lead a new chain.
    REQUIRE(action::ActivateEffect(d, ActionSpec{ActionId::LP_Damage, EffectType::Ignition, 1, 300}, 0)
                .find("Chain Link 1") != std::string::npos);
    // SS1 can never be Chain Link 2+.
    REQUIRE(action::ActivateEffect(d, ActionSpec{ActionId::LP_Gain, EffectType::Ignition, 1}, 1)
                .find("illegal chain") != std::string::npos);
    // A response must be at least as fast as the link it responds to.
    REQUIRE(action::ActivateEffect(d, ActionSpec{ActionId::LP_Gain, EffectType::Ignition, 2, 500}, 1)
                .find("Chain Link 2") != std::string::npos);
    REQUIRE(action::ActivateEffect(d, ActionSpec{ActionId::LP_Gain, EffectType::Ignition, 1}, 0)
                .find("illegal chain") != std::string::npos);
    // SS3 may answer SS2; then SS2 may NOT answer SS3.
    ActionArgs hitP1; // explicit victim: player 1
    hitP1.targetPlayer = 1;
    REQUIRE(action::ActivateEffect(d, ActionSpec{ActionId::LP_Damage, EffectType::Ignition, 3, 100}, 0,
                               hitP1)
                .find("Chain Link 3") != std::string::npos);
    REQUIRE(action::ActivateEffect(d, ActionSpec{ActionId::LP_Gain, EffectType::Ignition, 2}, 1)
                .find("illegal chain") != std::string::npos);

    // Resolve: last link first — the -100 (link 3) fires before the -300 (link 1).
    std::string log = action::ResolveChain(d);
    CHECK(d.chain.links.empty());
    REQUIRE(log.find("LP -100") != std::string::npos);
    REQUIRE(log.find("LP -300") != std::string::npos);
    CHECK(log.find("LP -100") < log.find("LP -300"));

    // LP math: p0 -300 (its own effect, no target named), p1 +500 (link 2)
    // then -100 (explicit target).
    CHECK(d.lp[0] == DuelConfig::START_LP - 300);
    CHECK(d.lp[1] == DuelConfig::START_LP + 500 - 100);

    // Cost_PayLP: the activator pays from its own LP.
    REQUIRE(action::ActivateEffect(d, ActionSpec{ActionId::Cost_PayLP, EffectType::Ignition, 1, 800}, 0)
                .find("Chain Link 1") != std::string::npos);
    action::ResolveChain(d);
    CHECK(d.lp[0] == DuelConfig::START_LP - 300 - 800);
}

// ── Win conditions (p.44) ────────────────────────────────────────────────────


// ── from tests.cpp lines 1194,1267 ──
TEST_CASE("Chain negation: a counter blanks the link it responds to (p.44)",
          "[engine][negate]") {
    Duel d;
    Card deck0[6], deck1[6];
    std::vector<Card *> dv0, dv1;
    for (int i = 0; i < 6; ++i) {
        dv0.push_back(mkMon(&deck0[i], 500 + i, 4, 1000, 800));
        dv1.push_back(mkMon(&deck1[i], 600 + i, 4, 900, 700));
    }
    action::SetDeck(d, 0, dv0);
    action::SetDeck(d, 1, dv1);
    action::DrawOpeningHands(d);
    action::StartTurn(d);

    // P0 opens with a Quick-Play style draw (SS 2) — Chain Link 1.
    CHECK(action::ActivateEffect(d, ActionSpec{ActionId::Move_Draw, EffectType::Ignition, 2, 2}, 0)
              .find("Chain Link 1") != std::string::npos);
    // P1 counters with a negation (SS 3) — Chain Link 2.
    CHECK(action::ActivateEffect(d, ActionSpec{ActionId::NegateActivation, EffectType::Quick, 3}, 1)
              .find("Chain Link 2") != std::string::npos);
    // SS 1 can never be Chain Link 2+ (p.41).
    CHECK(action::ActivateEffect(d, ActionSpec{ActionId::Move_MillToGY, EffectType::Ignition, 1}, 0)
              .find("illegal chain") != std::string::npos);

    const std::string log = action::ResolveChain(d);
    CHECK(log.find("negates Chain Link 1") != std::string::npos);
    CHECK(log.find("negated") != std::string::npos);
    // The draw was negated — the hand is unchanged.
    CHECK(d.field.handZones[0].count() == DuelConfig::START_HAND);

    // A negation with no open chain is rejected outright.
    CHECK(action::ActivateEffect(d, ActionSpec{ActionId::NegateEffect, EffectType::Quick, 3}, 0)
              .find("nothing to negate") != std::string::npos);

    // Cost_PayLP (Solemn-style) really mutates LP at resolution.
    CHECK(action::ActivateEffect(d, ActionSpec{ActionId::Cost_PayLP, EffectType::Ignition, 3, 1000}, 0)
              .find("Chain Link 1") != std::string::npos);
    action::ResolveChain(d);
    CHECK(d.lp[0] == DuelConfig::START_LP - 1000);
}

TEST_CASE("Summon_Flip link triggers the flipped card's effect (Man-Eater)",
          "[engine][flip]") {
    Duel d;
    Card flipper;
    mkMon(&flipper, 700, 3, 450, 600);
    flipper.name = "Man-Eater Bug"; // catalog: FLIP 500 damage to opponent

    // Face-down Defense on P0's side (Limited + Horizontal = set).
    flipper.state.owner = flipper.state.controller = 0;
    d.field.monsterZones[0][0].put(&flipper);
    d.field.monsterZones[0][0].changeVisibility(Visibility::Limited);
    d.field.monsterZones[0][0].changeOrientation(Orientation::Horizontal);

    action::StartTurn(d);
    d.turn.phase = Phase::Main1;

    ActionArgs fa;
    fa.target = &flipper;
    CHECK(action::ActivateEffect(d, ActionSpec{ActionId::Summon_Flip}, 0, fa)
              .find("Chain Link 1") != std::string::npos);
    const std::string log = action::ResolveChain(d);
    CHECK(log.find("flipped face-up") != std::string::npos);
    CHECK(log.find("Flip effects trigger") != std::string::npos);
    CHECK(log.find("LP -500") != std::string::npos);
    // The unnamed LP victim on a flip trigger is the controller's opponent.
    CHECK(d.lp[1] == DuelConfig::START_LP - 500);
    CHECK(d.lp[0] == DuelConfig::START_LP);
    CHECK(d.field.monsterZones[0][0].isVisible());
}


