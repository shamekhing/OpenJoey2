#pragma once
#include "ContentPaths.hpp"
#include "card/Card.hpp"
#include "game/zone/Field.hpp"
#include "game/zone/Zone.hpp"
#include "ui/AppScreen.hpp"
#include "ui/StyleSheet.hpp"
#include "ui/core/AppContext.hpp"
#include "ui/screens/IScreen.hpp"
#include "ui/widgets/display/CardPreview.hpp"
#include "ui/widgets/duel/FieldGrid.hpp"
#include "ui/widgets/duel/ZoneInfoPanel.hpp"
#include <filesystem>
#include <functional>
#include <iostream>
#include <raylib.h>
#include <string>
#include <vector>

namespace openjoey::ui {
using namespace openjoey::zone;

class DuelScreen : public IScreen {
public:
    explicit DuelScreen(AppContext& ctx) : ctx_(ctx) {
        loadCardBack();
        buildPool();
        fieldGrid_.build(field_);
        seedField();
        rebuildActions();
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

        drawHeader(0, 0, _SW, headerH);
        drawPreviewPanel(0, headerH, leftW, fieldH);

        const Texture2D* cb = cardBack_.id ? &cardBack_ : nullptr;
        fieldGrid_.draw({(float)leftW, (float)headerH, (float)centerW, (float)fieldH},
                        const_cast<Field&>(field_), ctx_.imageCache, cb);

        ZoneInfoPanel::Draw(
            {(float)(leftW + centerW), (float)headerH, (float)rightW, (float)fieldH},
            fieldGrid_.cursorZone(const_cast<Field&>(field_)),
            fieldGrid_.cursorLabel(const_cast<Field&>(field_)),
            actionLabels_, actionCursor_, lastResult_,
            fieldGrid_.selectedZone() != nullptr);

        drawFooter(0, _SH - footerH, _SW, footerH);
    }

private:
    AppContext&               ctx_;
    openjoey::zone::Field     field_;
    std::vector<openjoey::Card> pool_;
    Texture2D                 cardBack_ = {};
    int                       poolIdx_  = 0;
    mutable FieldGrid         fieldGrid_;

    struct Action {
        std::string                label;
        std::function<std::string()> invoke;
    };
    std::vector<Action>       actions_;
    std::vector<std::string>  actionLabels_;
    int                       actionCursor_ = 0;
    std::string               lastResult_;
    mutable CardPreview       preview_;
    std::string               numBuf_;

    // ── Setup
    void loadCardBack() {
        auto path = ContentPaths::cardBackImg();
        if (std::filesystem::exists(path))
            cardBack_ = LoadTexture(path.c_str());
        if (!cardBack_.id)
            std::cerr << "Failed to load card back image from " << path << "\n";
    }

    static openjoey::Card makeCard(const char* name, uint32_t num,
                                   const char* desc, CardType t,
                                   int atk, int def, int lvl) {
        openjoey::Card c;
        c.name        = name;
        c.cardNumber  = num;
        c.description = desc;
        c.type        = t;
        c.atk         = atk;
        c.def         = def;
        c.level       = lvl;
        return c;
    }

    void buildPool() {
        pool_ = {
            makeCard("Blue-Eyes",    89631139, "3000 ATK dragon.",        CardType::Monster, 3000, 2500, 8),
            makeCard("Dk Magician",  46986414, "2500 ATK spellcaster.",   CardType::Monster, 2500, 2100, 7),
            makeCard("Kuriboh",      40640057, "Reduce damage to 0.",     CardType::Monster,  300,  200, 1),
            makeCard("Raigeki",      12580477, "Destroy all opp mons.",   CardType::Spell,      0,    0, 0),
            makeCard("Reborn",       83764719, "Special Summon 1 mon.",   CardType::Spell,      0,    0, 0),
            makeCard("Mirror Force", 44095762, "Destroy ATK monsters.",   CardType::Trap,       0,    0, 0),
        };
    }

    void seedField() {
        for (auto& c : pool_) { c.owner = 1; c.controller = 1; }
        pool_[1].owner = 0; pool_[1].controller = 0;

        for (int i = 2; i < (int)pool_.size(); ++i)
            field_.deckZones[1].put(&pool_[i]);

        field_.monsterZones[1][2].put(&pool_[0]);
        field_.monsterZones[1][2].changeVisibility(Visibility::Visible);
        field_.monsterZones[1][2].changeOrientation(Orientation::Vertical);

        field_.monsterZones[0][2].put(&pool_[1]);
        field_.monsterZones[0][2].changeVisibility(Visibility::Visible);
        field_.monsterZones[0][2].changeOrientation(Orientation::Vertical);
    }

    // ── Input
    ScreenEvent handleInput() {
        float wheel = GetMouseWheelMove();
        if (wheel != 0.f) {
            preview_.scroll((int)wheel);
            return ScreenEvent::none();
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            numBuf_.clear();
            if (fieldGrid_.selectedZone()) {
                fieldGrid_.setSelectedZone(nullptr);
                lastResult_ = "Deselected.";
            } else {
                return ScreenEvent::replace(AppScreen::MainMenu);
            }
            return ScreenEvent::none();
        }

        if (IsKeyPressed(KEY_ENTER)) {
            if (!numBuf_.empty()) {
                int idx = std::stoi(numBuf_) - 1;
                numBuf_.clear();
                if (idx >= 0 && idx < (int)actions_.size()) {
                    lastResult_ = actions_[idx].invoke();
                    rebuildActions();
                } else {
                    lastResult_ = "No action at that index.";
                }
                return ScreenEvent::none();
            }
            handleEnter();
            return ScreenEvent::none();
        }

        if (IsKeyPressed(KEY_TAB)) {
            numBuf_.clear();
            if (!actions_.empty())
                actionCursor_ = (actionCursor_ + 1) % (int)actions_.size();
            return ScreenEvent::none();
        }

        if (IsKeyPressed(KEY_SPACE)) {
            numBuf_.clear();
            if (!actions_.empty()) {
                lastResult_ = actions_[actionCursor_].invoke();
                rebuildActions();
            }
            return ScreenEvent::none();
        }

        for (int k = KEY_ZERO; k <= KEY_NINE; ++k) {
            if (IsKeyPressed(k)) {
                if (numBuf_.size() < 2)
                    numBuf_ += static_cast<char>('0' + (k - KEY_ZERO));
                return ScreenEvent::none();
            }
        }

        int dr = 0, dc = 0;
        if (IsKeyPressed(KEY_UP))    dr = -1;
        if (IsKeyPressed(KEY_DOWN))  dr =  1;
        if (IsKeyPressed(KEY_LEFT))  dc = -1;
        if (IsKeyPressed(KEY_RIGHT)) dc =  1;
        if (dr || dc) {
            numBuf_.clear();
            fieldGrid_.moveCursor(dr, dc, field_);
            rebuildActions();
        }
        return ScreenEvent::none();
    }

    void handleEnter() {
        IZone* z = fieldGrid_.cursorZone(field_);
        if (!z) return;

        if (!fieldGrid_.selectedZone()) {
            if (!z->isEmpty()) {
                fieldGrid_.setSelectedZone(z);
                lastResult_ = "Source selected.";
            } else {
                lastResult_ = "Zone empty — nothing to pick.";
            }
        } else {
            if (z == fieldGrid_.selectedZone()) {
                fieldGrid_.setSelectedZone(nullptr);
                lastResult_ = "Deselected.";
                return;
            }
            lastResult_ = fieldGrid_.selectedZone()->moveTo(*z) ? "Moved OK." : "Move failed.";
            fieldGrid_.setSelectedZone(nullptr);
            rebuildActions();
        }
    }

    // ── Actions
    void rebuildActions() {
        actions_.clear();
        actionLabels_.clear();
        actionCursor_ = 0;
        IZone* z = fieldGrid_.cursorZone(field_);
        if (!z) return;

        auto push = [&](std::string lbl, std::function<std::string()> fn) {
            actions_.push_back({std::move(lbl), std::move(fn)});
        };

        push("put(card)", [this, z]() -> std::string {
            Card* c = nextPoolCard();
            return c ? (z->put(c) ? "put OK." : "put failed — zone full.")
                     : "Pool exhausted.";
        });
        push("remove()", [z]() -> std::string {
            return z->remove() ? "remove OK." : "remove failed — empty.";
        });
        push("reset()", [z]() -> std::string { z->reset(); return "reset OK."; });

        if (auto* zm = dynamic_cast<Zone_Monster*>(z)) {
            push("setATK  [Vertical]", [zm] {
                return zm->changeOrientation(Orientation::Vertical) ? "ATK OK." : "changeOrientation → false.";
            });
            push("setDEF  [Horizontal]", [zm] {
                return zm->changeOrientation(Orientation::Horizontal) ? "DEF OK." : "changeOrientation → false.";
            });
            push("vis: Visible  (both)", [zm] {
                return zm->changeVisibility(Visibility::Visible) ? "Visible OK." : "changeVisibility → false.";
            });
            push("vis: Limited  (owner)", [zm] {
                return zm->changeVisibility(Visibility::Limited) ? "Limited OK." : "changeVisibility → false.";
            });
            push("vis: Restricted (FD)", [zm] {
                return zm->changeVisibility(Visibility::Restricted) ? "Restricted OK." : "changeVisibility → false.";
            });
            push("flip()", [zm] {
                return zm->flip() ? "flip OK." : "flip → false (need Vertical+Limited).";
            });
        }

        push("contains(top)", [z]() -> std::string {
            Card* top = nullptr;
            if (auto* zm = dynamic_cast<Zone_Monster*>(z))      top = zm->peek();
            else if (auto* zs = dynamic_cast<ZoneStack*>(z))    top = zs->peek(-1);
            else if (auto* zn = dynamic_cast<Zone*>(z))         top = zn->peek();
            if (!top) return "contains: zone empty.";
            return z->contains(top) ? "contains(top) → true." : "contains(top) → false.";
        });
        push("moveTo→P1 GY", [this, z]() -> std::string {
            return z->moveTo(field_.graveyardZones[1]) ? "moveTo OK." : "moveTo failed.";
        });
        push("moveTo→P2 GY", [this, z]() -> std::string {
            return z->moveTo(field_.graveyardZones[0]) ? "moveTo OK." : "moveTo failed.";
        });

        if (auto* deck = dynamic_cast<ZoneStack_Deck*>(z)) {
            push("draw()→P1 hand", [this, deck] {
                return deck->draw(field_.handZones[1]) ? "draw OK." : "draw failed.";
            });
            push("mill(1)→P1 GY", [this, deck] {
                return deck->mill(1, field_.graveyardZones[1]) ? "mill OK." : "mill failed.";
            });
        }

        if (auto* zs = dynamic_cast<ZoneStack*>(z)) {
            push("peek(-1) top", [zs]() -> std::string {
                Card* c = zs->peek(-1);
                return c ? "peek(-1) → " + c->name : "peek(-1) → null.";
            });
            push("peek(0) bottom", [zs]() -> std::string {
                Card* c = zs->peek(0);
                return c ? "peek(0) → " + c->name : "peek(0) → null.";
            });
            push("findAll(monster)", [zs]() -> std::string {
                auto v = zs->findAll([](const Card* c) { return c->isMonster(); });
                return "findAll(monster) → " + std::to_string(v.size()) + " cards.";
            });
            push("findAll(!monster)", [zs]() -> std::string {
                auto v = zs->findAll([](const Card* c) { return !c->isMonster(); });
                return "findAll(!monster) → " + std::to_string(v.size()) + " cards.";
            });
            push("shuffle()", [zs]() -> std::string { zs->shuffle(); return "shuffle OK."; });
            push("clear()",   [zs]() -> std::string { zs->clear();   return "clear OK."; });
        }

        push("clearField()", [this]() -> std::string { field_.clearField(); return "clearField OK."; });
        push("cntMon P1", [this]() -> std::string {
            return "countMonsters(P1) → " + std::to_string(field_.countMonsters(1));
        });
        push("cntMon P2", [this]() -> std::string {
            return "countMonsters(P2) → " + std::to_string(field_.countMonsters(0));
        });
        push("fstEmptyMon P1", [this]() -> std::string {
            return "firstEmptyMonsterZone(P1) → " + std::to_string(field_.firstEmptyMonsterZone(1));
        });
        push("fstEmptyMon P2", [this]() -> std::string {
            return "firstEmptyMonsterZone(P2) → " + std::to_string(field_.firstEmptyMonsterZone(0));
        });
        push("fstEmptyST P1", [this]() -> std::string {
            return "firstEmptySpellTrapZone(P1) → " + std::to_string(field_.firstEmptySpellTrapZone(1));
        });
        push("fstEmptyST P2", [this]() -> std::string {
            return "firstEmptySpellTrapZone(P2) → " + std::to_string(field_.firstEmptySpellTrapZone(0));
        });
        push("fstOccupMon P1", [this]() -> std::string {
            return "firstOccupiedMonsterZone(P1) → " + std::to_string(field_.firstOccupiedMonsterZone(1));
        });
        push("fstOccupMon P2", [this]() -> std::string {
            return "firstOccupiedMonsterZone(P2) → " + std::to_string(field_.firstOccupiedMonsterZone(0));
        });
        push("fstEmptyEMZ", [this]() -> std::string {
            return "firstEmptyExtraMonsterZone() → " + std::to_string(field_.firstEmptyExtraMonsterZone());
        });

        for (auto& a : actions_) actionLabels_.push_back(a.label);
    }

    Card* nextPoolCard() {
        for (int i = 0; i < (int)pool_.size(); ++i) {
            Card* c = &pool_[poolIdx_];
            poolIdx_ = (poolIdx_ + 1) % (int)pool_.size();
            if (!inAnyZone(c)) return c;
        }
        return nullptr;
    }

    bool inAnyZone(Card* c) const {
        for (int p = 0; p < 2; ++p) {
            for (int i = 0; i < 5; ++i)
                if (field_.monsterZones[p][i].contains(c) ||
                    field_.spellTrapZones[p][i].contains(c)) return true;
            if (field_.graveyardZones[p].contains(c) ||
                field_.deckZones[p].contains(c) ||
                field_.extraDeckZones[p].contains(c) ||
                field_.fieldZones[p].contains(c) ||
                field_.banishedZones[p].contains(c) ||
                field_.handZones[p].contains(c)) return true;
        }
        return false;
    }

    // ── Draw
    void drawHeader(int x, int y, int w, int h) const {
        DrawRectangle(x, y, w, h, COLOR_HEADER_BG);
        DrawLine(x, y + h - 1, x + w, y + h - 1, COLOR_DIVIDER_LINE);
        int fs = FONT_SCREEN_TITLE;
        const char* mode = fieldGrid_.selectedZone()
            ? "SELECT DEST  [ESC=cancel]"
            : "NAVIGATE  [ENTER=pick source]";
        DrawText(("Duel Field  —  " + std::string(mode)).c_str(),
                 x + HEADER_TITLE_X, y + (h - fs) / 2, fs, YELLOW);
        const char* cl = fieldGrid_.cursorLabel(const_cast<Field&>(field_));
        int tw = MeasureText(cl, fs);
        DrawText(cl, x + w - tw - HEADER_TITLE_X, y + (h - fs) / 2, fs, COLOR_STAT_TEXT);
    }

    void drawFooter(int x, int y, int w, int h) const {
        DrawRectangle(x, y, w, h, COLOR_FOOTER_BG);
        DrawLine(x, y, x + w, y, COLOR_DIVIDER_LINE);
        int fs = FONT_HELP_TEXT;
        DrawText("Arrows:move  Enter:zone-select/move or confirm-#  "
                 "0-9:type action#  Tab:cycle  Space:exec-tab  Esc:back",
                 x + MAIN_PAD_X, y + (h - fs) / 2, fs, COLOR_STAT_TEXT);
        if (!numBuf_.empty()) {
            std::string nb = "  #" + numBuf_ + "_  ";
            int tw = MeasureText(nb.c_str(), fs);
            DrawRectangle(x + w - tw - MAIN_PAD_X, y + 1, tw, h - 2, Fade(YELLOW, 0.25f));
            DrawText(nb.c_str(), x + w - tw - MAIN_PAD_X, y + (h - fs) / 2, fs, YELLOW);
        }
    }

        // Delegates to the shared CardPreview widget (ui/widgets/display/CardPreview)
    // instead of duplicating the portrait/name/stat/wrap-scroll layout here.
    void drawPreviewPanel(int x, int y, int w, int h) const {
        preview_.SetCardBack(cardBack_.id ? &cardBack_ : nullptr);

        IZone* z   = fieldGrid_.cursorZone(const_cast<Field&>(field_));
        Card*  top = nullptr;
        bool   fd  = false;
        if (z && !z->isEmpty()) {
            if (auto* zm = dynamic_cast<Zone_Monster*>(z)) {
                top = zm->peek();
                fd  = zm->visibility() == Visibility::Restricted;
            } else if (auto* zs = dynamic_cast<ZoneStack*>(z)) {
                top = zs->peek(-1);
            } else if (auto* zn = dynamic_cast<Zone*>(z)) {
                top = zn->peek();
            }
        }
        preview_.SetCard(top, fd);
        preview_.Draw({(float)x, (float)y, (float)w, (float)h}, ctx_.imageCache);
    }
};

} // namespace openjoey::ui
