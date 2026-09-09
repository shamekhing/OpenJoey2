#pragma once
// ── Duel UI action types + shared screen state (openjoey::ui) ────────────────
// The hotseat state machine's vocabulary and every mutable duel-UI value in
// one small header. The components in ui/duel/ (DuelActions, DuelEffects,
// DuelPanels) share this state by reference; DuelScreen owns it. Every
// offered action carries a ActionId tag so the menu maps onto the
// rules vocabulary (openjoey-gameplay include/rules/RuleAction.hpp; UI-only
// entries carry RuleAction::None).

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "action/ActionId.hpp"
#include "action/ActionResult.hpp"
#include "cards/Card.hpp"
#include "engine/action/Catalog.hpp"  // ActionSpec (pending activation)
#include "ui/platform/Haptics.hpp"

namespace openjoey::ui {
using namespace openjoey::engine;
using cards::Card;
using cards::CardDatabase;

// Hotseat input state machine (used by DuelScreen).
enum class DuelMode : uint8_t {
    Navigate,       // free cursor movement over the field
    Menu,           // an action list is open for the cursor's zone
    AttackTarget,   // picking an attack target for the declared attacker
    EffectTarget,   // picking a target for a card activation
    TributeTarget,  // picking tribute / fusion / ritual materials
};

// One entry of the zone action menu: a label for the list UI, the engine
// call it runs (returns the structured verdict shown in the result strip),
// and the rules vocabulary entry it realizes.
struct DuelAction {
    std::string label;
    std::function<ActionResult()> invoke;
    ActionId id = ActionId::None;
};

using DuelActionList = std::vector<DuelAction>;

// All mutable duel-screen UI state. Plain data + one state helper; no engine
// or raylib dependencies, so components stay decoupled from each other.
struct DuelUIState {
    // Input mode.
    DuelMode mode = DuelMode::Navigate;

    // Action menu (DuelMode::Menu).
    DuelActionList actions;
    int actionCursor = 0;

    // Pending attack.
    Card* attacker = nullptr;

    // Pending card activation (spell/trap/monster effect in flight).
    Card* pendingCard = nullptr;    // the card being activated
    Card* pendingTarget = nullptr;  // target chosen in EffectTarget mode
    ActionSpec pendingFx{};         // its catalog entry
    int pendingOwner = 0;           // player who activated pendingFx

    // Chain responder window (the other player may chain; R resolves).
    bool chainPrompt = false;

    // Hotseat pass-the-device gate + controls popup.
    bool handoff = false;
    bool helpOpen = true;  // open on the first duel (H toggles)

    // Cards awaiting the Graveyard after chain resolution.
    std::vector<Card*> activated;

    // Tribute / fusion / ritual material picking.
    std::vector<Card*> tributePicks;
    int tributeCount = 0;
    bool fusionPending = false;
    bool ritualPending = false;

    // Status line (bottom of the info panel) + its verdict kind, which drives
    // the strip colour: engine refusals are red, successes green, UI prompts
    // neutral. The old code sniffed the text for "OK"/"true"/"Moved" — none of
    // the engine's result strings contained those, so every successful summon
    // rendered in the failure colour.
    enum class Feedback : uint8_t { Info, Ok, Fail };
    std::string lastResult;
    Feedback feedback = Feedback::Info;

    // Hotseat privacy: own hand renders as backs while on; press-and-hold the
    // hand strip peeks. (The HAND bar button toggles it.)
    bool hideHand = false;

    // ── Card-list overlay (compact chips: GY / banished / extra / deck) ──────
    // A snapshot of the zone at open time — input is blocked while it shows.
    bool listOpen = false;
    std::string listTitle;
    std::vector<Card*> listCards;
    int listScroll = 0;

    // Record an engine verdict; opens the chain responder window when the
    // engine reports a new chain link.
    void post(const ActionResult& r) {
        post(r.msg);
        feedback = r.msg.empty() ? Feedback::Info : (r.ok ? Feedback::Ok : Feedback::Fail);
        if (!r.msg.empty()) openjoey::ui::platform::hapticPulse(r.ok ? 12 : 35);
    }
    // Informational prompt (targeting hints etc.) — never a verdict colour.
    void post(const std::string& r) {
        feedback = Feedback::Info;
        lastResult = r;
        if (!r.empty()) {
            log.push_back(r);
            if (log.size() > 200) log.erase(log.begin(), log.begin() + (log.size() - 200));
        }
        if (r.find("Chain Link") != std::string::npos) chainPrompt = true;
    }

    // ── Duel log (L toggles the overlay) ─────────────────────────────────────
    // Every engine narration line lands here; a duel becomes followable.
    std::vector<std::string> log;
    bool logOpen = false;
};

}  // namespace openjoey::ui