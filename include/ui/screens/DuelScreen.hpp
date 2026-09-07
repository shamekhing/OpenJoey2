#pragma once
// ── Hotseat duel screen (classic rules, no AI) ───────────────────────────────
// Orchestration only: owns the duel state and the input state machine, and
// delegates everything else —
//   • bootstrap       → DuelSetup   (decks, seating, card back)
//   • action menu     → DuelActions (contextual RuleAction-tagged menu)
//   • effect plumbing → DuelEffects (chain push, set, graveyard sweep)
//   • chrome/overlay  → DuelPanels  (header, footer, gate, banner, help)
//   • field view      → FieldGrid   (layout, cursor, cell rendering)
// Both duelists are humans sharing one device. The engine (duel/Engine.hpp)
// enforces every rule; this screen only navigates, offers legal actions, and
// renders. The view always belongs to the turn player; between turns a
// pass-device gate hides hands from the other player.

#include "engine/duel/Duel.hpp"
#include "engine/duel/Engine.hpp"
#include "cards/Card.hpp"
#include "engine/field/Field.hpp"
#include "ui/AppScreen.hpp"
#include "ui/widgets/StyleSheet.hpp"
#include "ui/core/AppContext.hpp"
#include "ui/screens/IScreen.hpp"
#include "ui/duel/Action.hpp"
#include <ui/cards/CardPreview.hpp>
#include "ui/duel/DuelActions.hpp"
#include "ui/duel/DuelEffects.hpp"
#include "ui/duel/DuelPanels.hpp"
#include "ui/duel/DuelSetup.hpp"
#include "ui/duel/FieldGrid.hpp"
#include "ui/duel/FieldRows.hpp"
#include "ui/duel/ZoneInfoPanel.hpp"
#include <raylib.h>
#include <string>
#include <vector>

namespace openjoey::ui {
using namespace openjoey::engine;
using cards::Card;
using cards::CardDatabase;

class DuelScreen : public IScreen {
public:
    explicit DuelScreen(AppContext& ctx)
        : ctx_(ctx), engine_(duel_), field_(duel_.field), fx_(engine_, field_, ui_),
          act_(engine_, duel_, field_, fieldGrid_, fx_, ui_) {
        setupDuel();
    }

    ~DuelScreen() override {
        if (cardBack_.id) UnloadTexture(cardBack_);
    }

    ScreenEvent Update(float /*dt*/) override {
        ctx_.imageCache.PollAndLoad();
        return handleInput();
    }

    void Draw() const override {
        ClearBackground(COLOR_BG_DARK);

        int leftW   = _SW * DUEL_LEFT_W_PCT / 100;
        int rightW  = _SW * DUEL_RIGHT_W_PCT / 100;
        int centerW = _SW - leftW - rightW;
        int headerH = HEADER_HEIGHT;
        int footerH = int(0.03f * _SH);
        int fieldH  = _SH - headerH - footerH;

        DuelPanels::drawHeader(engine_, duel_, ui_, 0, 0, _SW, headerH);
        drawPreviewPanel(0, headerH, leftW, fieldH);

        // Legal-target highlights for the active pick mode (C: the board
        // shows what the rules allow instead of failing on confirm).
        fieldGrid_.clearHighlights();
        if (ui_.mode == DuelMode::AttackTarget) {
            for (auto& mz : field_.monsterZones[1 - fieldGrid_.viewer()])
                if (!mz.isEmpty()) fieldGrid_.addHighlight(&mz, RED);
        } else if (ui_.mode == DuelMode::EffectTarget) {
            for (int p = 0; p < 2; ++p) {
                for (auto& mz : field_.monsterZones[p])
                    if (!mz.isEmpty()) fieldGrid_.addHighlight(&mz, GREEN);
                for (auto& sz : field_.spellTrapZones[p])
                    if (!sz.isEmpty()) fieldGrid_.addHighlight(&sz, GREEN);
            }
        } else if (ui_.mode == DuelMode::TributeTarget) {
            for (auto& mz : field_.monsterZones[fieldGrid_.viewer()])
                if (!mz.isEmpty()) fieldGrid_.addHighlight(&mz, ORANGE);
        }

        const Texture2D* cb = cardBack_.id ? &cardBack_ : nullptr;
        fieldGrid_.draw({(float)leftW, (float)headerH, (float)centerW, (float)fieldH},
                        const_cast<Field&>(field_), ctx_.imageCache, cb);

        ZoneInfoPanel::Draw(
            {(float)(leftW + centerW), (float)headerH, (float)rightW, (float)fieldH},
            fieldGrid_.cursorZone(const_cast<Field&>(field_)),
            fieldGrid_.cursorLabel(const_cast<Field&>(field_)),
            ui_.actions, ui_.actionCursor, ui_.lastResult,
            fieldGrid_.selectedZone() != nullptr);

        DuelPanels::drawFooter(0, _SH - footerH, _SW, footerH);
        DuelPanels::drawOverlays(duel_, ui_);

        // Duel log overlay (L): the full narration, newest at the bottom.
        if (ui_.logOpen) {
            int lw = int(_SW * 0.62f), lh = int(_SH * 0.62f);
            int lx = (_SW - lw) / 2, ly = headerH + int(0.02f * _SH);
            DrawRectangle(lx, ly, lw, lh, Fade(BLACK, 0.88f));
            DrawRectangleLinesEx({(float)lx, (float)ly, (float)lw, (float)lh},
                                 2.f, GOLD);
            DrawText("DUEL LOG  [L close]", lx + 10, ly + 8, 18, GOLD);
            const int fs = 15, lineH = fs + 5;
            int maxLines = (lh - 40) / lineH;
            int start = std::max(0, (int)ui_.log.size() - maxLines);
            for (int i = start; i < (int)ui_.log.size(); ++i) {
                Color c = ui_.log[i].find("Chain Link") != std::string::npos
                              ? SKYBLUE
                          : ui_.log[i].find("destroys") != std::string::npos ||
                                  ui_.log[i].find("damage") != std::string::npos
                              ? ORANGE
                              : RAYWHITE;
                DrawText(ui_.log[i].substr(0, 110).c_str(), lx + 12,
                         ly + 34 + (i - start) * lineH, fs, c);
            }
        }
    }

private:
    AppContext& ctx_;
    Duel        duel_;
    Engine      engine_;
    Field&      field_;
    std::vector<Card> deckA_, deckB_;   // one main deck instance per player
    std::vector<Card> extraA_, extraB_; // fusion monsters routed out of the main deck
    Texture2D   cardBack_ = {};
    mutable FieldGrid fieldGrid_;
    DuelUIState ui_;                    // shared screen state (Action.hpp)
    DuelEffects fx_;                    // effect-activation plumbing
    DuelActions act_;                   // contextual action menu
    mutable CardPreview preview_;

    // ── Setup / turn flow ─────────────────────────────────────────────────────
    void setupDuel() {
        if (cardBack_.id) UnloadTexture(cardBack_);
        cardBack_ = DuelSetup::loadCardBack(ctx_);
        DuelSetup::buildDecks(ctx_, deckA_, deckB_, extraA_, extraB_);
        DuelSetup::seatDecks(engine_, field_, deckA_, deckB_, extraA_, extraB_);
        fieldGrid_.build(field_);
        // Per-duel format switches come from persisted settings.
        duel_.config.chainResponseWindow = ctx_.settings.chainResponseWindow;
        duel_.config.autoDiscardEndPhase = ctx_.settings.autoDiscardEndPhase;
        engine_.clearUndo();
        engine_.drawOpeningHands();
        engine_.startTurn();
        advanceToMain1(); // Draw Phase has no decisions — go straight to Main1
        fieldGrid_.setViewer(duel_.turnPlayer, field_);
        ui_.lastResult = "Duel start — player " + std::to_string(duel_.turnPlayer) +
                         " begins.";
        ui_.mode        = DuelMode::Navigate;
        ui_.chainPrompt = ui_.handoff = false;
        ui_.attacker = ui_.pendingCard = nullptr;
        ui_.activated.clear();
        ui_.tributePicks.clear();
        ui_.log.clear();
        ui_.logOpen = false;
        act_.rebuild();
    }

    void rematch() {
        duel_ = Duel{};
        engine_.hardReset();
        setupDuel();
        ui_.lastResult = "rematch — new duel started.";
    }

    // Draw Phase needs no input in this engine (startTurn already drew) — the
    // screen jumps straight to Main1 so every action menu is live at once.
    void advanceToMain1() { engine_.toMain1(); }

    void endTurnFlow() {
        if (ui_.chainPrompt) { ui_.lastResult = "resolve the chain first (R)."; return; }
        if (ui_.mode == DuelMode::AttackTarget) {
            ui_.lastResult = "cancel the attack first (ESC).";
            return;
        }
        std::string r = engine_.endTurn();
        r += " " + engine_.startTurn();
        advanceToMain1();
        fieldGrid_.setViewer(duel_.turnPlayer, field_);
        ui_.handoff = true; // SPACE gate hides the next player's hand
        ui_.lastResult = r;
    }

    // ── Input state machine ──────────────────────────────────────────────────
    // Arrows/WASD move · ENTER opens menus / confirms · ESC cancels
    // A attack · C change position · F flip summon / confirm tributes
    // B Battle Phase · N Main2 · E end turn · R resolve chain / rematch
    // SPACE confirms the pass-device gate.
    ScreenEvent handleInput() {
        if (duel_.result != DuelResult::Ongoing) {
            if (IsKeyPressed(KEY_R)) rematch();
            return ScreenEvent::none();
        }
        if (ui_.handoff) {
            if (IsKeyPressed(KEY_SPACE)) {
                ui_.handoff    = false;
                ui_.lastResult = "player " + std::to_string(duel_.turnPlayer + 1) +
                                 " — your turn.";
            }
            return ScreenEvent::none();
        }
        // Chain window: the other player may respond via card menus; R passes
        // / resolves. Engine-driven mode (p.45): both must pass before the
        // chain resolves, so the prompt stays open after the first pass.
        if (ui_.chainPrompt && IsKeyPressed(KEY_R)) {
            ui_.post(engine_.chainWaiting()
                         ? engine_.passResponse(1 - duel_.turnPlayer)
                         : engine_.resolveChain());
            if (duel_.chain.links.empty()) {
                fx_.sweepResolved();
                ui_.chainPrompt = false;
                ui_.mode        = DuelMode::Navigate;
            } else {
                ui_.lastResult += " — still open, the other player may chain.";
            }
            return ScreenEvent::none();
        }
        if (ui_.helpOpen) { // help modal blocks gameplay input; H closes it
            if (IsKeyPressed(KEY_H)) ui_.helpOpen = false;
            return ScreenEvent::none();
        }
        if (IsKeyPressed(KEY_H)) { ui_.helpOpen = true; return ScreenEvent::none(); }
        if (IsKeyPressed(KEY_L)) { ui_.logOpen = !ui_.logOpen; return ScreenEvent::none(); }
        if (IsKeyPressed(KEY_Z) && engine_.canUndo()) { // one-step undo
            engine_.undo();
            fieldGrid_.setViewer(duel_.turnPlayer, field_);
            ui_.mode = DuelMode::Navigate;
            act_.rebuild();
            ui_.lastResult = "undo — last action reverted.";
            return ScreenEvent::none();
        }

        const bool up  = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
        const bool down = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
        // Left/right stay live in every mode: targeting (attack/effect/tribute)
        // must be able to leave the attacker's column (p.35 target selection).
        const bool lf  = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
        const bool rt  = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
        bool ent = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
        bool esc = IsKeyPressed(KEY_ESCAPE);
        const int  dr  = (down ? 1 : 0) - (up ? 1 : 0);
        const int  dc  = (rt ? 1 : 0) - (lf ? 1 : 0);

        // ── Mouse: click-to-cursor; every click reuses a keyboard pathway ────
        // Left click moves the keyboard cursor onto the hit zone/hand card and
        // then acts like ENTER (open menu / confirm target). Right click acts
        // like ESC. Hovering (Navigate only) moves the cursor for inspection.
        const Vector2 mousePos = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            esc = true;
        } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (fieldGrid_.pointToCursor(mousePos, field_)) {
                if (ui_.mode == DuelMode::Menu)
                    ui_.mode = DuelMode::Navigate; // re-open fresh menu on the hit cell
                ent = true;
            } else if (ui_.mode == DuelMode::Menu) {
                esc = true; // click away from any zone closes the menu
            }
        } else if (ui_.mode == DuelMode::Navigate && !ui_.chainPrompt) {
            const Vector2 md = GetMouseDelta();
            if (md.x != 0.f || md.y != 0.f)
                fieldGrid_.pointToCursor(mousePos, field_); // hover-follow inspect
        }

        if (ui_.mode == DuelMode::Navigate) {
            if (IsKeyPressed(KEY_B)) {
                std::string r = engine_.toBattle();
                if (r == "Battle Phase.") r += " SPACE on your monster to attack.";
                ui_.post(r);
            }
            if (IsKeyPressed(KEY_N)) ui_.post(engine_.toMain2());
            if (IsKeyPressed(KEY_E)) { endTurnFlow(); return ScreenEvent::none(); }
        }

        switch (ui_.mode) {
        case DuelMode::Navigate: {
            fieldGrid_.moveCursor(dr, dc, field_);
            if (ent) { ui_.mode = DuelMode::Menu; act_.rebuild(); }
            if (IsKeyPressed(KEY_F)) { // flip summon own face-down monster
                Card* c = cursorCard();
                if (gridRow(FieldRow::OwnMonster) && c)
                    ui_.post(engine_.flipSummon(c));
                else ui_.lastResult = "flip summon: cursor on your face-down monster.";
            }
            if (IsKeyPressed(KEY_C)) { // change battle position (once/turn)
                Card* c = cursorCard();
                if (gridRow(FieldRow::OwnMonster) && c)
                    ui_.post(engine_.changePosition(c));
                else ui_.lastResult = "position change: cursor on your monster.";
            }
            if (IsKeyPressed(KEY_SPACE) && duel_.turn.phase == Phase::Battle)
                attackFlow();
            break;
        }

        case DuelMode::Menu: {
            if (up || down) {
                const int n = (int)ui_.actions.size();
                if (n > 0)
                    ui_.actionCursor = (ui_.actionCursor + (down ? 1 : n - 1)) % n;
            }
            if (esc) { ui_.mode = DuelMode::Navigate; break; }
            if (ent) {
                if (ui_.actions.empty()) { ui_.mode = DuelMode::Navigate; break; }
                const std::string r = ui_.actions[ui_.actionCursor].invoke();
                if (ui_.mode == DuelMode::Menu)
                    ui_.mode = DuelMode::Navigate; // actions may switch mode
                ui_.post(r);
            }
            break;
        }
        case DuelMode::AttackTarget: {
            if (esc) {
                engine_.cancelAttack();
                ui_.attacker   = nullptr;
                ui_.mode       = DuelMode::Navigate;
                ui_.lastResult = "attack called off.";
                break;
            }
            fieldGrid_.moveCursor(dr, dc, field_);
            if (ent) {
                Card* t = cursorCard();
                if (!gridRow(FieldRow::OppMonster)) {
                    ui_.lastResult = "pick a target on the OPPONENT's monster row "
                                     "(ENTER on the empty row = direct attack).";
                    break;
                }
                if (!t && !engine_.canDirectAttack(ui_.attacker)) {
                    ui_.lastResult = "opponent still controls monsters — pick one.";
                    break;
                }
                ui_.post(engine_.declareAttack(ui_.attacker, t));
                if (engine_.confirmAttack()) { // replay check (p.37)
                    ui_.post(engine_.resolveDamage());
                    fx_.sweepResolved();
                    ui_.mode = DuelMode::Navigate;
                } else {
                    ui_.lastResult += " REPLAY — target changed; pick again.";
                }
                ui_.attacker = nullptr;
            }
            break;
        }

        case DuelMode::EffectTarget: {
            if (esc) {
                ui_.pendingCard = ui_.pendingTarget = nullptr;
                ui_.mode        = DuelMode::Navigate;
                ui_.lastResult  = "activation cancelled.";
                break;
            }
            fieldGrid_.moveCursor(dr, dc, field_);
            if (ent) {
                Card* t           = cursorCard();
                ui_.pendingTarget = t;
                ui_.mode          = DuelMode::Navigate;
                ui_.post(fx_.finishActivation(t));
                fx_.sweepResolved();
            }
            break;
        }
        case DuelMode::TributeTarget: {
            if (esc) {
                ui_.tributePicks.clear();
                ui_.tributeCount  = 0;
                ui_.fusionPending = ui_.ritualPending = false;
                ui_.pendingCard   = nullptr;
                ui_.mode          = DuelMode::Navigate;
                ui_.lastResult    = "summon cancelled.";
                break;
            }
            fieldGrid_.moveCursor(dr, dc, field_);
            if (IsKeyPressed(KEY_F)) { act_.enterTributeConfirm(); break; }
            if (ent) {
                Card* c = cursorCard();
                if (gridRow(FieldRow::OwnMonster) && c &&
                    fieldGrid_.ownerOf(fieldGrid_.cursorZone(field_), field_) ==
                        fieldGrid_.viewer())
                    act_.toggleTributePick(c);
                else ui_.lastResult = "pick tributes on YOUR monster row.";
            }
            break;
        }
        default: break;
        }
        return ScreenEvent::none();
    }

    // ── Attack entry (SPACE in Battle Phase) ─────────────────────────────────
    void attackFlow() {
        Card* c = cursorCard();
        if (!c || !gridRow(FieldRow::OwnMonster)) {
            ui_.lastResult = "attack: cursor must be on YOUR monster row (A).";
            return;
        }
        if (!engine_.canAttack(c)) {
            ui_.lastResult = "attack not possible (Battle Phase, your face-up ATK "
                             "monster, once per Battle Phase).";
            return;
        }
        ui_.attacker = c;
        if (field_.countMonsters(1 - duel_.turnPlayer) == 0) {
            // Direct attack resolves immediately (p.34).
            ui_.post(engine_.declareAttack(c, nullptr));
            if (engine_.confirmAttack()) {
                ui_.post(engine_.resolveDamage());
                fx_.sweepResolved();
            }
            ui_.attacker = nullptr;
            ui_.mode     = DuelMode::Navigate;
            return;
        }
        ui_.mode       = DuelMode::AttackTarget;
        ui_.lastResult = c->name + " — pick an attack target (ENTER), ESC calls it off.";
    }

    // ── Cursor / view helpers ────────────────────────────────────────────────
    bool gridRow(FieldRow r) const {
        return fieldGrid_.cursorRow() == fieldRow(r);
    }
    Card* cursorCard() const {
        return fieldGrid_.cursorCard(const_cast<Field&>(field_));
    }

    // Entitlement: Visible = both; Limited = the card's owner only; Restricted
    // = neither. Drives the preview.
    bool canView(const Card* c) const {
        if (!c) return false;
        auto [z, p] = field_.findCard(const_cast<Card*>(c));
        if (!z) return true;
        const zone::Visibility v = z->visibility();
        return v == zone::Visibility::Visible ||
               (v == zone::Visibility::Limited &&
                c->state.controller == fieldGrid_.viewer());
    }

    void drawPreviewPanel(int x, int y, int w, int h) const {
        preview_.SetCardBack(cardBack_.id ? &cardBack_ : nullptr);
        zone::IZone* z   = fieldGrid_.cursorZone(const_cast<Field&>(field_));
        Card*        top = cursorCard(); // follows the cursor (was: zone top only)
        bool         fd  = false;
        if (z) {
            // Restricted zones (decks) never show; otherwise entitlement rules.
            if (z->visibility() == zone::Visibility::Restricted) fd = true;
            else if (top && !canView(top))                      fd = true;
        }
        preview_.SetCard(top, fd);
        preview_.Draw({(float)x, (float)y, (float)w, (float)h}, ctx_.imageCache);
    }
};

} // namespace openjoey::ui