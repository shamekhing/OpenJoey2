#pragma once
// ── Effect-activation plumbing for the duel screen (openjoey::ui) ────────────
// Builds resolver arguments from the classic catalog entry, pushes
// activations onto the chain, seats spells/traps that activate from the
// hand, and sweeps resolved cards to the Graveyard. Operates purely on the
// engine/field + DuelUIState — no raylib, no cursor.

#include <string>

#include "action/ActionArgs.hpp"  // ActionArgs
#include "cards/Card.hpp"
#include "engine/action/Catalog.hpp"
#include "engine/action/Moves.hpp"  // action::detail::moveCard
#include "engine/duel/Engine.hpp"
#include "engine/field/Field.hpp"
#include "ui/duel/Action.hpp"

namespace openjoey::ui {
using namespace openjoey::engine;
using cards::Card;
using cards::CardDatabase;

struct DuelEffects {
    Engine& engine;
    zone::Field& field;
    DuelUIState& st;

    DuelEffects(Engine& e, zone::Field& f, DuelUIState& s)
        : engine(e), field(f), st(s) {}

    // Push a spell/trap activation onto the chain: pay the cost, open the
    // chain link, park the card for the Graveyard after resolution, and open
    // the responder window.
    std::string finishActivation(Card* target) {
        // The engine charges spec.lpCost at activation (one place, never
        // refunded); scope on the spec derives mass targets — no sentinels.
        ActionArgs a;
        a.target = target;
        a.source = st.pendingCard;  // engine checks set-turn Traps (p.31)
        const std::string r =
            engine.activateEffect(st.pendingFx, st.pendingOwner, a);
        if (r.find("Chain Link") != std::string::npos) {
            auto [hz, hp] = field.findCard(st.pendingCard);
            if (hz && hz->type() == zone::ZoneType::Hand)
                setSpellTrap(st.pendingCard);  // spells sit in the S/T row while resolving
            st.activated.push_back(st.pendingCard);
            st.chainPrompt = true;
            st.mode = DuelMode::Navigate;
        }
        st.pendingCard = st.pendingTarget = nullptr;
        return r;
    }

    // Set a hand spell/trap face-down into its controller's first empty S/T zone.
    std::string setSpellTrap(Card* c) {
        if (!c) return "no card.";
        auto [z, p] = field.findCard(c);
        if (!z || z->type() != zone::ZoneType::Hand) return "card is not in your hand.";
        const int slot = field.firstEmptySpellTrapZone(c->state.controller);
        if (slot < 0) return "no free spell/trap zone.";
        if (!z->remove(c)) return "remove from hand failed.";
        auto& stz = field.spellTrapZones[c->state.controller][slot];
        if (!stz.put(c)) {
            z->put(c);
            return "set failed — zone rejected the card.";
        }
        stz.changeVisibility(zone::Visibility::Limited);  // face-down: owner only
        return c->name + " set face-down.";
    }

    // Spells/Traps that resolved sit in the S/T row until they hit the GY.
    void sweepResolved() {
        for (Card* c : st.activated) {
            auto [z, p] = field.findCard(c);
            if (z && z->type() == zone::ZoneType::SpellTrap)
                action::detail::moveCard(field, c, field.graveyardZones[c->state.controller]);
        }
        st.activated.clear();
    }
};

}  // namespace openjoey::ui