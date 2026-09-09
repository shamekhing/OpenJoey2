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

#include <raylib.h>

#include <cmath>
#include <string>
#include <ui/cards/CardPreview.hpp>
#include <vector>

#include "cards/Card.hpp"
#include "engine/duel/Duel.hpp"
#include "engine/duel/Engine.hpp"
#include "engine/field/Field.hpp"
#include "ui/AppScreen.hpp"
#include "ui/core/AppContext.hpp"
#include "ui/duel/Action.hpp"
#include "ui/duel/DuelActions.hpp"
#include "ui/duel/DuelEffects.hpp"
#include "ui/duel/DuelLayout.hpp"
#include "ui/duel/DuelPanels.hpp"
#include "ui/duel/DuelSetup.hpp"
#include "ui/duel/FieldGrid.hpp"
#include "ui/duel/FieldRows.hpp"
#include "ui/duel/ZoneInfoPanel.hpp"
#include "ui/input/TouchInput.hpp"
#include "ui/screens/IScreen.hpp"
#include "ui/widgets/StyleSheet.hpp"

namespace openjoey::ui {
using namespace openjoey::engine;
using cards::Card;
using cards::CardDatabase;

class DuelScreen : public IScreen {
   public:
    explicit DuelScreen(AppContext& ctx) : ctx_(ctx), engine_(duel_), field_(duel_.field), fx_(engine_, field_, ui_), act_(engine_, duel_, field_, fieldGrid_, fx_, ui_) { setupDuel(); }

    ~DuelScreen() override {
        if (cardBack_.id) UnloadTexture(cardBack_);
    }

    ScreenEvent Update(float /*dt*/) override {
        ctx_.imageCache.PollAndLoad();
        return handleInput();
    }

    void Draw() const override {
        ClearBackground(COLOR_BG_DARK);

        // Compact screens (phones): the side panels would squeeze the mat to
        // ~66px zones. The mat takes the full width; card details live on
        // long-press and the verdict strip stays in the action sheet.
        const bool compact = _SW < 820;
        int leftW = compact ? 0 : _SW * DUEL_LEFT_W_PCT / 100;
        int rightW = compact ? 0 : _SW * DUEL_RIGHT_W_PCT / 100;
        int centerW = _SW - leftW - rightW;
        fieldGrid_.setCompact(compact);
        int headerH = HEADER_HEIGHT;
        int footerH = int(0.03f * _SH);
        const int barH = (int)DuelLayout::barH();
        int fieldH = _SH - headerH - footerH - barH;

        DuelPanels::drawHeader(engine_, duel_, ui_, 0, 0, _SW, headerH);
        if (!compact) drawPreviewPanel(0, headerH, leftW, fieldH);

        // Legal-target highlights for the active pick mode (C: the board
        // shows what the rules allow instead of failing on confirm).
        fieldGrid_.clearHighlights();
        if (ui_.mode == DuelMode::AttackTarget) {
            for (auto& mz : field_.monsterZones[1 - fieldGrid_.viewer()])
                if (!mz.isEmpty()) fieldGrid_.addHighlight(&mz, RED);
            // Direct attack: the empty opponent row is a legal target too —
            // light it up, otherwise the one cell you must tap has no
            // affordance at all (the old code highlighted only occupants).
            if (ui_.attacker && engine_.canDirectAttack(ui_.attacker)) {
                const unsigned char a = (unsigned char)(140 + 110 * (0.5f + 0.5f * sinf((float)GetTime() * 6.f)));
                for (auto& mz : field_.monsterZones[1 - fieldGrid_.viewer()])
                    if (mz.isEmpty()) fieldGrid_.addHighlight(&mz, {250, 200, 40, a});
            }
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
        const bool peekHand = IsMouseButtonDown(MOUSE_BUTTON_LEFT) && fieldGrid_.ownHandHit(GetMousePosition());
        fieldGrid_.draw({(float)leftW, (float)headerH, (float)centerW, (float)fieldH}, const_cast<Field&>(field_), ctx_.imageCache, cb, ui_.hideHand && !peekHand);

        if (rightW > 0) ZoneInfoPanel::Draw({(float)(leftW + centerW), (float)headerH, (float)rightW, (float)fieldH}, fieldGrid_.cursorZone(const_cast<Field&>(field_)), fieldGrid_.cursorLabel(const_cast<Field&>(field_)), ui_.feedback, ui_.lastResult, fieldGrid_.selectedZone() != nullptr);

        // ── Bottom action sheet: ONE menu UI for keyboard, mouse and touch ──
        // Keyboard: arrows move actionCursor, ENTER invokes. Mouse/touch: tap
        // a row. Rows keep a >=48px height and the window auto-centres on the
        // cursor, so nothing is selectable-but-invisible (the old right-panel
        // list allowed selecting entries it had scrolled out of view).
        if (ui_.mode == DuelMode::Menu && !ui_.actions.empty()) {
            const int visible = sheetVisible();
            const int first = sheetFirst();
            const Rectangle sh = DuelLayout::sheetRect(visible);
            DrawRectangleRec(sh, {16, 16, 26, 246});
            DrawLine((int)sh.x, (int)sh.y, (int)(sh.x + sh.width), (int)sh.y, COLOR_DIVIDER_MID);
            const int fs = 0.026f * _SH < 15 ? 15 : (int)(0.026f * _SH);
            for (int row = 0; row < visible; ++row) {
                const int idx = first + row;
                const Rectangle rr = DuelLayout::sheetRowRect(visible, row);
                const bool sel = idx == ui_.actionCursor;
                if (sel) DrawRectangleRec(rr, Fade(GOLD, 0.14f));
                DrawLine((int)rr.x, (int)rr.y, (int)(rr.x + rr.width), (int)rr.y, COLOR_DIVIDER_LINE);
                DrawText(DrawUtils::clipText(ui_.actions[idx].label, (int)(sh.width - 24), fs).c_str(), (int)rr.x + 12, (int)(rr.y + (rr.height - fs) / 2), fs, sel ? GOLD : RAYWHITE);
            }
            if (first > 0 || first + visible < (int)ui_.actions.size()) {
                const char* hint = TextFormat("%d/%d", ui_.actionCursor + 1, (int)ui_.actions.size());
                DrawText(hint, (int)(sh.x + sh.width - MeasureText(hint, 13) - 10), (int)sh.y - 17, 13, COLOR_STAT_TEXT);
            }
        }
        if (ui_.mode != DuelMode::Navigate) drawCancelButton();

        DuelPanels::drawActionBar(engine_, duel_, ui_);
        DuelPanels::drawFooter(0, _SH - footerH - barH, _SW, footerH);
        DuelPanels::drawOverlays(duel_, ui_);

        // Duel log overlay (L): the full narration, newest at the bottom.
        if (ui_.logOpen) {
            int lw = int(_SW * 0.62f), lh = int(_SH * 0.62f);
            int lx = (_SW - lw) / 2, ly = headerH + int(0.02f * _SH);
            DrawRectangle(lx, ly, lw, lh, Fade(BLACK, 0.88f));
            DrawRectangleLinesEx({(float)lx, (float)ly, (float)lw, (float)lh}, 2.f, GOLD);
            DrawText("DUEL LOG  [L close]", lx + 10, ly + 8, 18, GOLD);
            const int fs = 15, lineH = fs + 5;
            int maxLines = (lh - 40) / lineH;
            int start = std::max(0, (int)ui_.log.size() - maxLines);
            for (int i = start; i < (int)ui_.log.size(); ++i) {
                Color c = ui_.log[i].find("Chain Link") != std::string::npos ? SKYBLUE : ui_.log[i].find("destroys") != std::string::npos || ui_.log[i].find("damage") != std::string::npos ? ORANGE : RAYWHITE;
                DrawText(ui_.log[i].substr(0, 110).c_str(), lx + 12, ly + 34 + (i - start) * lineH, fs, c);
            }
        }

        // ── Card-list overlay: tap a peripheral chip (compact) to audit a zone.
        // Snapshotted at open time; entitlements apply (face-down stays hidden).
        if (ui_.listOpen) {
            DrawRectangle(0, 0, (float)_SW, (float)_SH, {0, 0, 0, 215});
            const Rectangle p = DuelLayout::listPanelRect();
            DrawRectangleRec(p, {18, 20, 28, 250});
            DrawRectangleLinesEx(p, 2.f, GOLD);
            DrawText(ui_.listTitle.c_str(), (int)(p.x + 14), (int)(p.y + 12), FONT_PANEL_TITLE, GOLD);
            const int fs = 14;
            const int rowH = 64;
            const int visRows = (int)((p.height - 46) / rowH);
            const int maxStart = std::max(0, (int)ui_.listCards.size() - visRows);
            const int start = std::min(std::max(ui_.listScroll, 0), maxStart);
            for (int row = 0; row < visRows && start + row < (int)ui_.listCards.size(); ++row) {
                Card* c = ui_.listCards[start + row];
                const float ty = p.y + 40 + (float)row * (float)rowH;
                const Rectangle thumb{p.x + 12, ty, 42.f, 42.f * 86.f / 59.f};
                const Texture2D* tex = canView(c) ? ctx_.imageCache.Get(*c) : nullptr;
                if (tex && tex->id) DrawTexturePro(*tex, {0, 0, (float)tex->width, (float)tex->height}, thumb, {0, 0}, 0.f, WHITE);
                else DrawRectangleRec(thumb, COLOR_BG_DARK);
                const bool seen = canView(c);
                DrawText(DrawUtils::clipText(seen ? c->name : "(face-down)", (int)(p.width - 96), fs).c_str(), (int)(p.x + 64), (int)(ty + 10), fs, seen ? RAYWHITE : COLOR_STAT_TEXT);
            }
            if (ui_.listCards.empty()) DrawText("(empty)", (int)(p.x + 14), (int)(p.y + 46), FONT_PANEL_TITLE, COLOR_STAT_TEXT);
            const Rectangle xc = DuelLayout::listCloseButton();
            DrawRectangleRec(xc, {20, 20, 30, 230});
            DrawRectangleLinesEx(xc, 1.5f, Color{230, 90, 90, 255});
            DrawText("X", (int)(xc.x + (xc.width - MeasureText("X", 20)) / 2), (int)(xc.y + (xc.height - 20) / 2), 20, Color{240, 130, 130, 255});
        }

        // ── Long-press card detail: hold any card ~0.35s to read it full-screen
        // (the touch replacement for hover-inspect; works on hands, fields and
        // list overlays). Entitlement rules still apply — face-down opponent
        // cards stay unreadable.
        if (press_.held()) {
            Card* c = fieldGrid_.cardAt(GetMousePosition(), const_cast<Field&>(field_));
            if (c && canView(c)) {
                DrawRectangle(0, 0, (float)_SW, (float)_SH, {0, 0, 0, 215});
                const float w = _SW * 0.86f, h = _SH * 0.80f;
                const Rectangle r{(float)(_SW - w) / 2.f, (float)(_SH - h) / 2.f, w, h};
                detail_.SetCardBack(cardBack_.id ? &cardBack_ : nullptr);
                detail_.SetCard(c, false);
                detail_.Draw(r, ctx_.imageCache);
                const char* hint = "release to close";
                DrawText(hint, (int)((_SW - MeasureText(hint, FONT_HELP_SMALL)) / 2), (int)(r.y + r.height + 8), FONT_HELP_SMALL, COLOR_STAT_TEXT);
            }
        }
    }

   private:
    AppContext& ctx_;
    Duel duel_;
    Engine engine_;
    Field& field_;
    std::vector<Card> deckA_, deckB_;    // one main deck instance per player
    std::vector<Card> extraA_, extraB_;  // fusion monsters routed out of the main deck
    Texture2D cardBack_ = {};
    mutable FieldGrid fieldGrid_;
    DuelUIState ui_;   // shared screen state (Action.hpp)
    DuelEffects fx_;   // effect-activation plumbing
    DuelActions act_;  // contextual action menu
    mutable CardPreview preview_;
    // Full-screen card inspector for the long-press gesture (Stage 2).
    mutable CardPreview detail_;
    PressTracker press_;

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
        advanceToMain1();  // Draw Phase has no decisions — go straight to Main1
        fieldGrid_.setViewer(duel_.turnPlayer, field_);
        ui_.lastResult = "Duel start — player " + std::to_string(duel_.turnPlayer) + " begins.";
        ui_.mode = DuelMode::Navigate;
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

    // ── Factored flows: every GUI button routes through the SAME code path ──
    // as the keyboard shortcut it replaces, so desktop behaviour is unchanged.
    void goBattle() {
        ActionResult r = engine_.toBattle();
        if (r.ok && r.msg == "Battle Phase.") r.msg += " SPACE on your monster to attack — or tap it.";
        ui_.post(r);
    }
    void goMain2() { ui_.post(engine_.toMain2()); }
    void resolveChainFlow() {
        ui_.post(engine_.chainWaiting() ? engine_.passResponse(1 - duel_.turnPlayer) : engine_.resolveChain());
        if (duel_.chain.links.empty()) {
            fx_.sweepResolved();
            ui_.chainPrompt = false;
            ui_.mode = DuelMode::Navigate;
        } else {
            ui_.lastResult += " — still open, the other player may chain.";
        }
    }
    void undoFlow() {
        if (!engine_.canUndo()) {
            ui_.lastResult = "nothing to undo.";
            return;
        }
        engine_.undo();
        fieldGrid_.setViewer(duel_.turnPlayer, field_);
        ui_.mode = DuelMode::Navigate;
        act_.rebuild();
        ui_.lastResult = "undo — last action reverted.";
    }
    void invokeMenuAction(int idx) {
        if (idx < 0 || idx >= (int)ui_.actions.size()) return;
        ui_.actionCursor = idx;
        const ActionResult r = ui_.actions[idx].invoke();
        if (ui_.mode == DuelMode::Menu) ui_.mode = DuelMode::Navigate;
        ui_.post(r);
        // A targeting prompt is an instruction, not a verdict — keep it out
        // of the green/red verdict colours.
        if (ui_.mode != DuelMode::Navigate) ui_.feedback = DuelUIState::Feedback::Info;
    }
    // Chip tap (compact layout): open a card-list overlay for a peripheral
    // zone. The deck stays sealed; everything else lists with entitlements.
    void openChipList(const FieldGrid::Chip& chip) {
        auto* zs = dynamic_cast<zone::ZoneStack*>(chip.zone);
        if (!zs) {
            ui_.lastResult = "nothing to inspect.";
            return;
        }
        if (chip.zone->visibility() == zone::Visibility::Restricted) {
            ui_.lastResult = "the deck is face-down — its contents are hidden.";
            return;
        }
        ui_.listCards.clear();
        for (int i = 0; i < zs->count(); ++i)
            if (Card* c = zs->peek(i)) ui_.listCards.push_back(c);
        ui_.listTitle = std::string(chip.label) + " — " + std::to_string(ui_.listCards.size());
        ui_.listScroll = 0;
        ui_.listOpen = true;
    }
    bool clicked(const Rectangle& r) const { return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), r); }
    // ── Action-sheet scroll window (shared by Draw and input) ────────────────
    int sheetVisible() const {
        const int n = (int)ui_.actions.size();
        const int maxRows = DuelLayout::sheetMaxRows();
        return n < maxRows ? n : maxRows;
    }
    int sheetFirst() const {
        int first = ui_.actionCursor - sheetVisible() / 2;
        if (first > (int)ui_.actions.size() - sheetVisible()) first = (int)ui_.actions.size() - sheetVisible();
        return first < 0 ? 0 : first;
    }
    void drawCancelButton() const {
        const Rectangle r = DuelLayout::cancelRect();
        DrawRectangleRec(r, {20, 20, 30, 230});
        DrawRectangleLinesEx(r, 1.5f, Color{230, 90, 90, 255});
        const char* x = "X";
        const int fs = (int)(r.height * 0.55f);
        DrawText(x, (int)(r.x + (r.width - MeasureText(x, fs)) / 2), (int)(r.y + (r.height - fs) / 2), fs, Color{240, 130, 130, 255});
    }

    void endTurnFlow() {
        if (ui_.chainPrompt) {
            ui_.lastResult = "resolve the chain first (R).";
            return;
        }
        if (ui_.mode == DuelMode::AttackTarget) {
            ui_.lastResult = "cancel the attack first (ESC).";
            return;
        }
        const ActionResult ended = engine_.endTurn();
        if (!ended.ok) {
            // Refused (hand limit unresolved, duel over) — the turn does NOT
            // pass, so the next player's turn must not start either. The old
            // code started it anyway, silently re-dealing the same player's turn.
            ui_.post(ended);
            return;
        }
        const ActionResult started = engine_.startTurn();
        advanceToMain1();
        fieldGrid_.setViewer(duel_.turnPlayer, field_);
        ui_.handoff = true;  // SPACE gate hides the next player's hand
        ui_.post(ActionResult(ended.ok && started.ok, ended.msg + " " + started.msg));
    }

    // ── Input state machine ──────────────────────────────────────────────────
    // Arrows/WASD move · ENTER opens menus / confirms · ESC cancels
    // A attack · C change position · F flip summon / confirm tributes
    // B Battle Phase · N Main2 · E end turn · R resolve chain / rematch
    // SPACE confirms the pass-device gate.
    ScreenEvent handleInput() {
        press_.update();  // long-press tracking (card detail overlay)
        if (duel_.result != DuelResult::Ongoing) {
            if (IsKeyPressed(KEY_R) || clicked(DuelLayout::winButton(0))) rematch();
            else if (clicked(DuelLayout::winButton(1))) return ScreenEvent::replace(AppScreen::MainMenu);
            return ScreenEvent::none();
        }
        if (ui_.handoff) {
            if (IsKeyPressed(KEY_SPACE) || clicked(DuelLayout::handoffButton())) {
                ui_.handoff = false;
                ui_.lastResult = "player " + std::to_string(duel_.turnPlayer + 1) + " — your turn.";
            }
            return ScreenEvent::none();
        }
        // Chain window: the other player may respond via card menus; R passes
        // / resolves. Engine-driven mode (p.45): both must pass before the
        // chain resolves, so the prompt stays open after the first pass.
        if (ui_.chainPrompt && (IsKeyPressed(KEY_R) || clicked(DuelPanels::chainButtonRect(duel_)))) {
            resolveChainFlow();
            return ScreenEvent::none();
        }
        if (ui_.helpOpen) {  // help modal blocks gameplay input; H or GOT IT closes
            if (IsKeyPressed(KEY_H) || clicked(DuelLayout::helpOkButton())) ui_.helpOpen = false;
            return ScreenEvent::none();
        }
        if (ui_.listOpen) {  // card-list overlay consumes everything while open
            if (clicked(DuelLayout::listCloseButton()) || IsKeyPressed(KEY_ESCAPE) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !CheckCollisionPointRec(GetMousePosition(), DuelLayout::listPanelRect()))) ui_.listOpen = false;
            ui_.listScroll -= (int)(GetMouseWheelMove() * 3.f);
            return ScreenEvent::none();
        }
        if (IsKeyPressed(KEY_H)) {
            ui_.helpOpen = true;
            return ScreenEvent::none();
        }
        if (IsKeyPressed(KEY_L)) {
            ui_.logOpen = !ui_.logOpen;
            return ScreenEvent::none();
        }
        if (IsKeyPressed(KEY_Z)) undoFlow();  // one-step undo (button in the bar too)

        const bool up = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
        const bool down = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
        // Left/right stay live in every mode: targeting (attack/effect/tribute)
        // must be able to leave the attacker's column (p.35 target selection).
        const bool lf = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
        const bool rt = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
        bool ent = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
        bool esc = IsKeyPressed(KEY_ESCAPE);
        const int dr = (down ? 1 : 0) - (up ? 1 : 0);
        const int dc = (rt ? 1 : 0) - (lf ? 1 : 0);

        // ── GUI buttons (mouse/touch) — routed through the same flows as the
        // keys. ✕ reuses each mode's cancel path; the action bar mirrors
        // B/N/E/Z/L; sheet rows invoke the menu action they render. A click
        // consumed here must not also reach the field hit-test below.
        bool guiClick = false;
        if (ui_.mode != DuelMode::Navigate && clicked(DuelLayout::cancelRect())) {
            esc = true;
            guiClick = true;
        }
        for (int slot = 0; slot < DuelLayout::kBarButtons; ++slot) {
            if (!clicked(DuelLayout::barButton(slot))) continue;
            if (!DuelPanels::barEnabled(engine_, duel_, slot)) return ScreenEvent::none();  // disabled buttons swallow the tap
            if (slot == 0) {
                if (duel_.turn.phase == Phase::Battle) goMain2();
                else goBattle();
            } else if (slot == 1) {
                endTurnFlow();
            } else if (slot == 2) {
                undoFlow();
            } else if (slot == 3) {
                ui_.logOpen = !ui_.logOpen;
            } else {
                ui_.hideHand = !ui_.hideHand;
            }
            return ScreenEvent::none();
        }
        if (ui_.mode == DuelMode::Navigate) {
            // Compact chips (deck/extra/GY/banish/field) open card lists.
            if (const FieldGrid::Chip* chip = fieldGrid_.chipHit(GetMousePosition())) {
                openChipList(*chip);
                return ScreenEvent::none();
            }
        }
        if (ui_.mode == DuelMode::Menu && !ui_.actions.empty()) {
            const int visible = sheetVisible();
            const int first = sheetFirst();
            for (int row = 0; row < visible; ++row) {
                if (clicked(DuelLayout::sheetRowRect(visible, row))) {
                    invokeMenuAction(first + row);
                    return ScreenEvent::none();
                }
            }
        }

        // ── Mouse: click-to-cursor; every click reuses a keyboard pathway ────
        // Left click moves the keyboard cursor onto the hit zone/hand card and
        // then acts like ENTER (open menu / confirm target). Right click acts
        // like ESC. Hovering (Navigate only) moves the cursor for inspection.
        const Vector2 mousePos = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            esc = true;
        } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !guiClick) {
            if (fieldGrid_.pointToCursor(mousePos, field_)) {
                if (ui_.mode == DuelMode::Menu) ui_.mode = DuelMode::Navigate;  // re-open fresh menu on the hit cell
                ent = true;
            } else if (ui_.mode == DuelMode::Menu) {
                esc = true;  // click away from any zone closes the menu
            }
        } else if (ui_.mode == DuelMode::Navigate && !ui_.chainPrompt) {
            const Vector2 md = GetMouseDelta();
            if (md.x != 0.f || md.y != 0.f) fieldGrid_.pointToCursor(mousePos, field_);  // hover-follow inspect
        }

        if (ui_.mode == DuelMode::Navigate) {
            if (IsKeyPressed(KEY_B)) goBattle();
            if (IsKeyPressed(KEY_N)) goMain2();
            if (IsKeyPressed(KEY_E)) {
                endTurnFlow();
                return ScreenEvent::none();
            }
        }

        switch (ui_.mode) {
            case DuelMode::Navigate: {
                fieldGrid_.moveCursor(dr, dc, field_);
                if (ent) {
                    ui_.mode = DuelMode::Menu;
                    act_.rebuild();
                }
                if (IsKeyPressed(KEY_F)) {  // flip summon own face-down monster
                    Card* c = cursorCard();
                    if (gridRow(FieldRow::OwnMonster) && c) ui_.post(engine_.flipSummon(c));
                    else ui_.lastResult = "flip summon: cursor on your face-down monster.";
                }
                if (IsKeyPressed(KEY_C)) {  // change battle position (once/turn)
                    Card* c = cursorCard();
                    if (gridRow(FieldRow::OwnMonster) && c) ui_.post(engine_.changePosition(c));
                    else ui_.lastResult = "position change: cursor on your monster.";
                }
                if (IsKeyPressed(KEY_SPACE) && duel_.turn.phase == Phase::Battle) attackFlow();
                break;
            }

            case DuelMode::Menu: {
                if (up || down) {
                    const int n = (int)ui_.actions.size();
                    if (n > 0) ui_.actionCursor = (ui_.actionCursor + (down ? 1 : n - 1)) % n;
                }
                if (esc) {
                    ui_.mode = DuelMode::Navigate;
                    break;
                }
                if (ent) { invokeMenuAction(ui_.actionCursor); }
                break;
            }
            case DuelMode::AttackTarget: {
                if (esc) {
                    engine_.cancelAttack();
                    ui_.attacker = nullptr;
                    ui_.mode = DuelMode::Navigate;
                    ui_.lastResult = "attack called off.";
                    break;
                }
                fieldGrid_.moveCursor(dr, dc, field_);
                if (ent) {
                    Card* t = cursorCard();
                    if (!gridRow(FieldRow::OppMonster)) {
                        ui_.lastResult =
                            "tap an opponent monster (an empty row = direct "
                            "attack).";
                        break;
                    }
                    if (!t && !engine_.canDirectAttack(ui_.attacker)) {
                        ui_.lastResult = "opponent still controls monsters — pick one.";
                        break;
                    }
                    ui_.post(engine_.declareAttack(ui_.attacker, t));
                    if (engine_.confirmAttack()) {  // replay check (p.37)
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
                    ui_.mode = DuelMode::Navigate;
                    ui_.lastResult = "activation cancelled.";
                    break;
                }
                fieldGrid_.moveCursor(dr, dc, field_);
                if (ent) {
                    Card* t = cursorCard();
                    ui_.pendingTarget = t;
                    ui_.mode = DuelMode::Navigate;
                    ui_.post(fx_.finishActivation(t));
                    fx_.sweepResolved();
                }
                break;
            }
            case DuelMode::TributeTarget: {
                if (esc) {
                    ui_.tributePicks.clear();
                    ui_.tributeCount = 0;
                    ui_.fusionPending = ui_.ritualPending = false;
                    ui_.pendingCard = nullptr;
                    ui_.mode = DuelMode::Navigate;
                    ui_.lastResult = "summon cancelled.";
                    break;
                }
                fieldGrid_.moveCursor(dr, dc, field_);
                if (IsKeyPressed(KEY_F)) {
                    act_.enterTributeConfirm();
                    break;
                }
                if (ent) {
                    Card* c = cursorCard();
                    if (gridRow(FieldRow::OwnMonster) && c && fieldGrid_.ownerOf(fieldGrid_.cursorZone(field_), field_) == fieldGrid_.viewer()) act_.toggleTributePick(c);
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
            ui_.lastResult = "attack: select a monster in YOUR monster row.";
            return;
        }
        if (!engine_.canAttack(c)) {
            ui_.lastResult =
                "attack not possible (Battle Phase, your face-up ATK "
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
            ui_.mode = DuelMode::Navigate;
            return;
        }
        ui_.mode = DuelMode::AttackTarget;
        ui_.lastResult = c->name + " — pick an attack target (ENTER), ESC calls it off.";
    }

    // ── Cursor / view helpers ────────────────────────────────────────────────
    bool gridRow(FieldRow r) const { return fieldGrid_.cursorRow() == fieldRow(r); }
    Card* cursorCard() const { return fieldGrid_.cursorCard(const_cast<Field&>(field_)); }

    // Entitlement: Visible = both; Limited = the card's owner only; Restricted
    // = neither. Drives the preview.
    bool canView(const Card* c) const {
        if (!c) return false;
        auto [z, p] = field_.findCard(const_cast<Card*>(c));
        if (!z) return true;
        const zone::Visibility v = z->visibility();
        return v == zone::Visibility::Visible || (v == zone::Visibility::Limited && c->state.controller == fieldGrid_.viewer());
    }

    void drawPreviewPanel(int x, int y, int w, int h) const {
        preview_.SetCardBack(cardBack_.id ? &cardBack_ : nullptr);
        zone::IZone* z = fieldGrid_.cursorZone(const_cast<Field&>(field_));
        Card* top = cursorCard();  // follows the cursor (was: zone top only)
        bool fd = false;
        if (z) {
            // Restricted zones (decks) never show; otherwise entitlement rules.
            if (z->visibility() == zone::Visibility::Restricted) fd = true;
            else if (top && !canView(top)) fd = true;
        }
        preview_.SetCard(top, fd);
        preview_.Draw({(float)x, (float)y, (float)w, (float)h}, ctx_.imageCache);
    }
};

}  // namespace openjoey::ui