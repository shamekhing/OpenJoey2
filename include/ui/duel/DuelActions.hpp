#pragma once
// ── Contextual action menu for the duel screen (openjoey::ui) ────────────────
// One function per context: hand monster, hand spell/trap, field zones, plus
// the fusion/ritual pick menus. Every gameplay action carries its
// ActionId id (None for UI-only pick lists).

#include <functional>
#include <string>
#include <vector>

#include "action/ActionId.hpp"
#include "cards/Card.hpp"
#include "engine/action/Catalog.hpp"
#include "engine/duel/Duel.hpp"
#include "engine/duel/Engine.hpp"
#include "engine/field/Field.hpp"
#include "ui/duel/Action.hpp"
#include "ui/duel/DuelEffects.hpp"
#include "ui/duel/FieldGrid.hpp"
#include "ui/duel/FieldRows.hpp"

namespace openjoey::ui {
using namespace openjoey::engine;
using cards::Card;
using cards::CardDatabase;

struct DuelActions {
    DuelActions(Engine& e, Duel& d, zone::Field& f, FieldGrid& g, DuelEffects& x, DuelUIState& s) : engine(e), duel(d), field(f), grid(g), fx(x), st(s) {}

    Engine& engine;
    Duel& duel;
    zone::Field& field;
    FieldGrid& grid;
    DuelEffects& fx;
    DuelUIState& st;

    // Rebuild the menu for whatever the cursor sits on (DuelMode::Menu only).
    void rebuild() {
        clear();
        if (st.mode != DuelMode::Menu) return;
        const int me = grid.viewer();
        zone::IZone* z = grid.cursorZone(field);
        Card* c = grid.cursorCard(field);
        if (grid.cursorRow() == fieldRow(FieldRow::OwnHand) && c) {
            handActions(c);
            return;
        }
        fieldActions(z, me);
    }

    void clear() {
        st.actions.clear();
        st.actionCursor = 0;
    }

    // F in TributeTarget mode: confirm picks → fusion/ritual menu, or run
    // the plain tribute summon.
    void enterTributeConfirm() {
        if (st.fusionPending) {
            fusionPickMenu();
            return;
        }
        if (st.ritualPending) {
            ritualPickMenu();
            return;
        }
        if ((int)st.tributePicks.size() != st.tributeCount) {
            st.lastResult = "need " + std::to_string(st.tributeCount) + " tributes.";
            return;
        }
        Card* c = st.pendingCard;
        st.pendingCard = nullptr;
        st.mode = DuelMode::Navigate;
        st.post(engine.tributeSummon(c, st.tributePicks));
        st.tributePicks.clear();
    }

    // ENTER in TributeTarget mode: toggle the hovered own monster as material.
    void toggleTributePick(Card* c) {
        if (!c) return;
        const int cap = st.fusionPending ? 2 : (st.ritualPending ? 99 : st.tributeCount);
        std::size_t at = st.tributePicks.size();
        for (std::size_t i = 0; i < st.tributePicks.size(); ++i)
            if (st.tributePicks[i] == c) {
                at = i;
                break;
            }
        if (at < st.tributePicks.size()) {
            st.tributePicks.erase(st.tributePicks.begin() + at);
            st.lastResult = "unpicked " + c->name + ".";
        } else if ((int)st.tributePicks.size() < cap) {
            st.tributePicks.push_back(c);
            st.lastResult = "picked " + c->name + " (" + std::to_string(st.tributePicks.size()) + ").";
        } else {
            st.lastResult = "enough tributes already picked.";
        }
    }

   private:
    void push(std::string lbl, std::function<ActionResult()> fn, ActionId id = ActionId::None) { st.actions.push_back({std::move(lbl), std::move(fn), id}); }

    // ── Own hand ─────────────────────────────────────────────────────────────
    void handActions(Card* c) {
        if (c->isMonster()) handMonsterActions(c);
        else if (c->isSpell() || c->isTrap()) handSpellTrapActions(c);
    }

    void handMonsterActions(Card* c) {
        if (!engine.canNormalSummon()) return;
        const int tr = Engine::tributesRequired(c);
        if (tr == 0) {
            push("normal summon (ATK)", [this, c] { return engine.normalSummon(c); }, ActionId::NormalSummon);
            push("normal set (face-down DEF)", [this, c] { return engine.normalSet(c); }, ActionId::NormalSet);
        } else {
            push(
                "tribute summon — needs " + std::to_string(tr),
                [this, c, tr] {
                    st.tributePicks.clear();
                    st.tributeCount = tr;
                    st.fusionPending = st.ritualPending = false;
                    st.pendingCard = c;
                    st.mode = DuelMode::TributeTarget;
                    return ActionResult::Ok("pick " + std::to_string(tr) + " tributes on your monsters.");
                },
                ActionId::TributeSummon);
        }
    }

    void handSpellTrapActions(Card* c) {
        const ActionSpec* e = findClassicEffect(c->name);
        if (e) {
            if (e->id == ActionId::Summon_Fusion) {
                push("fusion: pick 2 field materials", [this, c] {
                    st.pendingCard = c;
                    st.tributePicks.clear();
                    st.tributeCount = 2;
                    st.fusionPending = true;
                    st.ritualPending = false;
                    st.mode = DuelMode::TributeTarget;
                    return ActionResult::Ok("pick 2 materials from your monster row.");
                });
            } else if (e->id == ActionId::Summon_Ritual) {
                push("ritual: pick tributes (F to confirm)", [this, c] {
                    st.pendingCard = c;
                    st.tributePicks.clear();
                    st.tributeCount = 0;
                    st.ritualPending = true;
                    st.fusionPending = false;
                    st.mode = DuelMode::TributeTarget;
                    return ActionResult::Ok("pick field tributes, then press F.");
                });
            } else {
                pushActivate(c, e);
            }
        }
        // Set is ALWAYS offered for hand spells/traps (fx or not) — when a
        // spell/trap zone is free (engine-backed check, p.27).
        if (field.firstEmptySpellTrapZone(grid.viewer()) >= 0) push("set in spell/trap zone", [this, c] { return fx.setSpellTrap(c); }, c->isTrap() ? ActionId::SetTrapCard : ActionId::SetSpellCard);
    }

    // Shared activation entry: arm the pending state, then target or fire.
    void pushActivate(Card* c, const ActionSpec* e) {
        const int me = grid.viewer();
        push(
            std::string("activate — ") + (e->note ? e->note : "effect"),
            [this, c, e, me]() -> ActionResult {
                st.pendingCard = c;
                st.pendingFx = *e;
                st.pendingOwner = me;
                st.pendingTarget = nullptr;
                if (e->needsTarget) {
                    st.mode = DuelMode::EffectTarget;
                    return ActionResult::Ok("pick a target.");
                }
                return fx.finishActivation(nullptr);
            },
            c->isMonster() ? ActionId::ActivateMonsterEffect :
            c->isTrap()    ? ActionId::ActivateTrapEffect :
                             ActionId::ActivateSpellEffect);
    }

    // ── Field zones (viewer-relative rows 1–4) ───────────────────────────────
    // Every offered action is gated by an ENGINE predicate (Can*), so the
    // menu can never offer something the rules reject and never hide
    // something legal.
    void fieldActions(zone::IZone* z, int me) {
        auto* zm = dynamic_cast<zone::Zone_Monster*>(z);
        auto* zst = dynamic_cast<zone::Zone_SpellTrap*>(z);
        Card* mc = zm ? zm->peek() : (zst ? zst->peek() : nullptr);
        const bool ownZone = (grid.ownerOf(z, field) == me);

        if (zm && mc && ownZone && mc->state.controller == me) {
            if (engine.canAttack(mc)) {  // Battle Phase, face-up ATK, unused attack
                push("declare attack", [this, mc] {
                    st.attacker = mc;
                    st.mode = DuelMode::AttackTarget;
                    return engine.canDirectAttack(mc) ? ActionResult::Ok("tap an empty opponent zone = direct attack.") : ActionResult::Ok("pick an opponent monster.");
                });
            }
            if (engine.canFlipSummon(mc)) {  // face-down Set, not on arrival turn
                push("flip summon", [this, mc] { return engine.flipSummon(mc); }, ActionId::FlipSummon);
            }
            if (engine.canChangePosition(mc)) {  // face-up, once/turn, not after attacking
                push("switch position (1/turn)", [this, mc] { return engine.changePosition(mc); }, ActionId::ChangeMonsterBattlePosition);
            }
        }

        if (zst && mc) {
            const ActionSpec* e = findClassicEffect(mc->name);
            // Only offer chaining when the response window is actually ON —
            // with it off the engine resolves immediately and "chain this
            // card" advertised a window that did not exist.
            if (st.chainPrompt && duel.config.chainResponseWindow && e) {
                push("chain this card", [this, mc, e] {
                    ActionArgs ca;
                    ca.source = mc;  // engine rejects set-this-turn Traps (p.31)
                    ActionResult r = engine.activateEffect(*e, mc->state.controller, ca);
                    if (r.ok) st.activated.push_back(mc);
                    return r;
                });
            } else if (e && ownZone && engine.canActivateFromZone(mc)) {
                pushActivate(mc, e);  // activate a set card from its zone (p.31)
            }
        }
    }

    // ── Fusion / ritual pick menus (TributeTarget + F) ───────────────────────
    void fusionPickMenu() {
        if ((int)st.tributePicks.size() != 2) {
            st.lastResult = "need exactly 2 materials (F when picked).";
            return;
        }
        clear();
        auto& ed = field.extraDeckZones[grid.viewer()];
        std::vector<Card*> seen;
        for (int i = 0; i < ed.count(); ++i) {
            Card* f = ed.peek(i);
            if (!f || !f->isMonster()) continue;
            bool dup = false;
            for (Card* s : seen)
                if (s->name == f->name) dup = true;
            if (dup) continue;
            seen.push_back(f);
            push(std::string("fusion summon — ") + f->name, [this, f] {
                st.fusionPending = false;
                ActionResult r = engine.fusionSummon(f, st.tributePicks);
                st.tributePicks.clear();
                return r;
            });
        }
        if (st.actions.empty()) {
            st.lastResult = "no fusion monster in your Extra Deck.";
            return;
        }
        st.mode = DuelMode::Menu;
    }

    void ritualPickMenu() {
        if (st.tributePicks.empty()) {
            st.lastResult = "pick at least 1 tribute (F when picked).";
            return;
        }
        clear();
        auto& h = field.handZones[grid.viewer()];
        for (int i = 0; i < h.count(); ++i) {
            Card* m = h.peek(i);
            if (!m || !m->isMonster()) continue;
            push(std::string("ritual summon — ") + m->name, [this, m] {
                st.ritualPending = false;
                ActionResult r = engine.ritualSummon(m, st.tributePicks);
                st.tributePicks.clear();
                return r;
            });
        }
        if (st.actions.empty()) {
            st.lastResult = "no ritual monster in your hand.";
            return;
        }
        st.mode = DuelMode::Menu;
    }
};

}  // namespace openjoey::ui
