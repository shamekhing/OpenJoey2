#pragma once
#include "ContentPaths.hpp"
#include "card/Card.hpp"
#include "game/zone/Field.hpp"
#include "game/zone/Zone.hpp"
#include "ui/core/AppContext.hpp"
#include "ui/core/AppScreen.hpp"
#include "ui/core/Theme.hpp"
#include "ui/screens/IScreen.hpp"
#include "ui/widgets/display/CardPreview.hpp"
#include "ui/widgets/duel/FieldGrid.hpp"
#include "ui/widgets/duel/ZoneInfoPanel.hpp"
#include "ui/widgets/layout/Popup.hpp"
#include "ui/widgets/layout/ScreenChrome.hpp"
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <raylib.h>
#include <string>
#include <vector>

namespace openjoey::ui {
using namespace openjoey::zone;

class DuelScreen : public IScreen {
public:
  explicit DuelScreen(AppContext &ctx) : ctx_(ctx) {
    loadCardBack();
    loadDuelFieldBg();
    loadSelectedDeck();
    fieldGrid_.build(field_);
    rebuildActions();
    preview_.SetCardBack(&cardBack_);
  }

  ~DuelScreen() override {
    if (cardBack_.id)
      UnloadTexture(cardBack_);
    if (duelBg_.id)
      UnloadTexture(duelBg_);
  }

  ScreenEvent Update(float /*dt*/) override {
    ctx_.imageCache.PollAndLoad();
    return handleInput();
  }

  void Draw() const override {
    const Theme t = Theme::FromScreen();
    ClearBackground(t.colors.bgDark);

    int centerW = std::min(t.sw, t.sh); // t.sw - leftW;
    int leftW = t.sw - centerW;         //* t.duelLeftWPct / 100;
    int headerH = t.headerHeight;
    int footerH = t.duelFooterH;
    int fieldH = t.sh - headerH - footerH;

    const char *modeStr = fieldGrid_.selectedZone()
                              ? "SELECT DEST  [ESC=cancel]"
                              : "NAVIGATE  [ENTER=pick source]";
    std::string headerTitle = std::string("Duel Field  ") + modeStr;
    const char *zoneLabel = fieldGrid_.cursorLabel(const_cast<Field &>(field_));
    ScreenChrome::DrawHeader(0, 0, t.sw, headerH, headerTitle.c_str(),
                             zoneLabel, YELLOW, t);

    {
      IZone *z = fieldGrid_.cursorZone(const_cast<Field &>(field_));
      preview_.SetCard(z->peek(), !z->canView(1));
      preview_.Draw({0.f, (float)headerH, (float)leftW, (float)fieldH},
                    ctx_.imageCache);
    }

    if (duelBg_.id)
      DrawTexturePro(
          duelBg_, {0, 0, (float)duelBg_.width, (float)duelBg_.height},
          {(float)leftW, (float)headerH, (float)centerW, (float)fieldH}, {0, 0},
          0.f, Fade(WHITE, t.duelFieldBgAlpha));

    const Texture2D *cb = cardBack_.id ? &cardBack_ : nullptr;
    fieldGrid_.draw(
        {(float)leftW, (float)headerH, (float)centerW, (float)fieldH},
        const_cast<Field &>(field_), ctx_.imageCache, cb, t);

    ScreenChrome::DrawFooter(
        0, t.sh - footerH, t.sw, footerH,
        "Arrows:move  Enter:select/move  H:actions  B:banished  Esc:back", t);

    if (showBanZone_) {
      Rectangle r =
          Popup::Begin(t.banPopupW, t.banPopupH, "Banished Zone   [B] close",
                       t.colors.banZoneAccent, t);
      int pad = t.previewPadX, bodyFs = t.fontCardStat, lineH = bodyFs + 6;
      int cx = (int)r.x, cy = (int)r.y, colW = t.banPopupW / 2;
      DrawLine(cx + pad / 2, cy, cx + t.banPopupW - pad / 2, cy,
               t.colors.dividerLine);
      cy += pad / 2;
      auto drawBanColumn = [&](IZone *zone, int ox) {
        if (!zone)
          return;
        DrawText(TextFormat("%s Banished: %d card(s)", ox == cx ? "P2" : "P1",
                            zone->count()),
                 ox + pad, cy, bodyFs, t.colors.statText);
        int iy = cy + lineH, shown = 0;
        if (auto *zs = dynamic_cast<ZoneStack *>(zone)) {
          for (int i = zs->count() - 1; i >= 0 && shown < 8; --i, ++shown) {
            Card *c = zs->peek(i);
            if (!c)
              continue;
            Color col = c->isMonster() ? t.colors.monsterStat
                        : c->isSpell() ? t.colors.spellStat
                                       : t.colors.trapStat;
            DrawText(c->name.c_str(), ox + pad, iy, bodyFs, col);
            iy += lineH;
          }
        } else if (!zone->isEmpty()) {
          if (auto *zn = dynamic_cast<Zone *>(zone)) {
            Card *c = zn->peek();
            if (c)
              DrawText(c->name.c_str(), ox + pad, iy, bodyFs,
                       t.colors.descText);
          }
        }
      };
      drawBanColumn(fieldGrid_.banishedZone(0), cx);
      drawBanColumn(fieldGrid_.banishedZone(1), cx + colW);
      Popup::End();
    }

    if (showHelp_) {
      Rectangle r =
          Popup::Begin(t.helpPopupW, t.helpPopupH, "Actions / Help  [H] close",
                       t.colors.popupBorder, t);
      if (!numBuf_.empty()) {
        std::string nb = "  #" + numBuf_ + "_";
        int tw = MeasureText(nb.c_str(), t.fontPanelTitle);
        DrawText(nb.c_str(), t.helpPopupX + t.helpPopupW - tw - t.previewPadX,
                 t.helpPopupY + t.previewPadX, t.fontPanelTitle,
                 t.colors.popupBorder);
      }
      ZoneInfoPanel::Draw(r, fieldGrid_.cursorZone(const_cast<Field &>(field_)),
                          fieldGrid_.cursorLabel(const_cast<Field &>(field_)),
                          actionLabels_, actionCursor_, lastResult_,
                          fieldGrid_.selectedZone() != nullptr);
      Popup::End();
    }
  }

private:
  AppContext &ctx_;
  openjoey::zone::Field field_;
  std::vector<openjoey::Card> pool_; // owns cards for the duration of the duel
  Texture2D cardBack_ = {};
  Texture2D duelBg_ = {};
  int poolIdx_ = 0;
  mutable FieldGrid fieldGrid_;
  mutable CardPreview preview_;
  bool showHelp_ = false;
  bool showBanZone_ = false;
  std::string numBuf_;

  struct Action {
    std::string label;
    std::function<std::string()> invoke;
  };
  std::vector<Action> actions_;
  std::vector<std::string> actionLabels_;
  int actionCursor_ = 0;
  std::string lastResult_;

  // ── Setup ────────────────────────────────────────────────────────────────

  void loadCardBack() {
    auto path = ContentPaths::cardBackImg();
    if (std::filesystem::exists(path))
      cardBack_ = LoadTexture(path.c_str());
    if (!cardBack_.id)
      std::cerr << "[DuelScreen] card back not found: " << path << "\n";
  }

  void loadDuelFieldBg() {
    auto path = ContentPaths::duelFieldBgImg();
    if (std::filesystem::exists(path))
      duelBg_ = LoadTexture(path.c_str());
  }

  void loadSelectedDeck() {
    if (ctx_.selectedDeck.empty())
      loadDeckFromFile("default");
    else
      pool_ = ctx_.selectedDeck;

    for (auto &c : pool_) {
      c.owner = 1;
      c.controller = 1;
    }
    for (auto &c : pool_)
      field_.deckZones[1].put(&c);
  }

  void loadDeckFromFile(const std::string &name) {
    std::filesystem::path path =
        std::filesystem::current_path() / "data" / "decks" / (name + ".txt");
    std::ifstream f(path);
    if (!f.is_open()) {
      std::cerr << "[DuelScreen] no saved deck at: " << path << "\n";
      return;
    }
    std::string line;
    while (std::getline(f, line)) {
      if (line.empty() || line[0] == '#')
        continue;
      try {
        uint32_t id = (uint32_t)std::stoul(line);
        const auto *card = ctx_.cardDb.GetCardById(id);
        if (card && (int)pool_.size() < 60)
          pool_.push_back(*card);
      } catch (...) {
      }
    }
  }

  // ── Input ─────────────────────────────────────────────────────────────────

  ScreenEvent handleInput() {
    if (showHelp_)
      return handleHelpInput();
    if (showBanZone_)
      return handleBanInput();
    return handleFieldInput();
  }

  ScreenEvent handleHelpInput() {
    if (IsKeyPressed(KEY_H) || IsKeyPressed(KEY_ESCAPE)) {
      showHelp_ = false;
      numBuf_.clear();
      return ScreenEvent::none();
    }
    if (IsKeyPressed(KEY_TAB)) {
      if (!actions_.empty())
        actionCursor_ = (actionCursor_ + 1) % (int)actions_.size();
      return ScreenEvent::none();
    }
    if (IsKeyPressed(KEY_SPACE)) {
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
    if (IsKeyPressed(KEY_ENTER) && !numBuf_.empty()) {
      int idx = std::stoi(numBuf_) - 1;
      numBuf_.clear();
      if (idx >= 0 && idx < (int)actions_.size()) {
        lastResult_ = actions_[idx].invoke();
        rebuildActions();
      } else {
        lastResult_ = "No action at that index.";
      }
    }
    if (IsKeyPressed(KEY_UP)) {
      if (!actions_.empty())
        actionCursor_ =
            (actionCursor_ - 1 + (int)actions_.size()) % (int)actions_.size();
      return ScreenEvent::none();
    }
    if (IsKeyPressed(KEY_DOWN)) {
      if (!actions_.empty())
        actionCursor_ = (actionCursor_ + 1) % (int)actions_.size();
      return ScreenEvent::none();
    }
    return ScreenEvent::none();
  }

  ScreenEvent handleBanInput() {
    if (IsKeyPressed(KEY_B) || IsKeyPressed(KEY_ESCAPE)) {
      showBanZone_ = false;
      return ScreenEvent::none();
    }
    return ScreenEvent::none();
  }

  ScreenEvent handleFieldInput() {
    float wheel = GetMouseWheelMove();
    if (wheel != 0.f) {
      preview_.scroll((int)wheel);
      return ScreenEvent::none();
    }

    if (IsKeyPressed(KEY_H)) {
      showHelp_ = true;
      return ScreenEvent::none();
    }
    if (IsKeyPressed(KEY_B)) {
      showBanZone_ = true;
      return ScreenEvent::none();
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
      if (fieldGrid_.selectedZone()) {
        fieldGrid_.setSelectedZone(nullptr);
        lastResult_ = "Deselected.";
      } else {
        return ScreenEvent::replace(AppScreen::MainMenu);
      }
      return ScreenEvent::none();
    }

    if (IsKeyPressed(KEY_ENTER)) {
      handleEnter();
      return ScreenEvent::none();
    }

    int dr = 0, dc = 0;
    if (IsKeyPressed(KEY_UP))
      dr = -1;
    if (IsKeyPressed(KEY_DOWN))
      dr = 1;
    if (IsKeyPressed(KEY_LEFT))
      dc = -1;
    if (IsKeyPressed(KEY_RIGHT))
      dc = 1;
    if (dr || dc) {
      fieldGrid_.moveCursor(dr, dc, field_);
      rebuildActions();
    }
    return ScreenEvent::none();
  }

  void handleEnter() {
    IZone *z = fieldGrid_.cursorZone(field_);
    if (!z)
      return;

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
      lastResult_ =
          fieldGrid_.selectedZone()->moveTo(*z) ? "Moved OK." : "Move failed.";
      fieldGrid_.setSelectedZone(nullptr);
      rebuildActions();
    }
  }

  // ── Actions ───────────────────────────────────────────────────────────────

  void rebuildActions() {
    actions_.clear();
    actionLabels_.clear();
    actionCursor_ = 0;
    IZone *z = fieldGrid_.cursorZone(field_);
    if (!z)
      return;

    auto push = [&](std::string lbl, std::function<std::string()> fn) {
      actions_.push_back({std::move(lbl), std::move(fn)});
    };

    push("put(card)", [this, z]() -> std::string {
      Card *c = nextPoolCard();
      return c ? (z->put(c) ? "put OK." : "put failed — zone full.")
               : "Pool exhausted.";
    });
    push("remove()", [z]() -> std::string {
      return z->remove() ? "remove OK." : "remove failed — empty.";
    });
    push("reset()", [z]() -> std::string {
      z->reset();
      return "reset OK.";
    });

    if (auto *zm = dynamic_cast<Zone_Monster *>(z)) {
      push("setATK  [Vertical]", [zm] {
        return zm->changeOrientation(Orientation::Vertical)
                   ? "ATK OK."
                   : "changeOrientation false.";
      });
      push("setDEF  [Horizontal]", [zm] {
        return zm->changeOrientation(Orientation::Horizontal)
                   ? "DEF OK."
                   : "changeOrientation false.";
      });
      push("vis: Visible  (both)", [zm] {
        return zm->changeVisibility(Visibility::Visible)
                   ? "Visible OK."
                   : "changeVisibility false.";
      });
      push("vis: Limited  (owner)", [zm] {
        return zm->changeVisibility(Visibility::Limited)
                   ? "Limited OK."
                   : "changeVisibility false.";
      });
      push("vis: Restricted (FD)", [zm] {
        return zm->changeVisibility(Visibility::Restricted)
                   ? "Restricted OK."
                   : "changeVisibility false.";
      });
      push("flip()", [zm] {
        return zm->flip() ? "flip OK." : "flip false (need Vertical+Limited).";
      });
    }

    push("contains(top)", [z]() -> std::string {
      Card *top = nullptr;
      if (auto *zm = dynamic_cast<Zone_Monster *>(z))
        top = zm->peek();
      else if (auto *zs = dynamic_cast<ZoneStack *>(z))
        top = zs->peek(-1);
      else if (auto *zn = dynamic_cast<Zone *>(z))
        top = zn->peek();
      if (!top)
        return "contains: zone empty.";
      return z->contains(top) ? "contains(top) true." : "contains(top) false.";
    });
    push("moveTo P1 GY", [this, z]() -> std::string {
      return z->moveTo(field_.graveyardZones[1]) ? "moveTo OK."
                                                 : "moveTo failed.";
    });
    push("moveTo P2 GY", [this, z]() -> std::string {
      return z->moveTo(field_.graveyardZones[0]) ? "moveTo OK."
                                                 : "moveTo failed.";
    });
    push("moveTo P1 Banished", [this, z]() -> std::string {
      return z->moveTo(field_.banishedZones[1]) ? "moveTo OK."
                                                : "moveTo failed.";
    });
    push("moveTo P2 Banished", [this, z]() -> std::string {
      return z->moveTo(field_.banishedZones[0]) ? "moveTo OK."
                                                : "moveTo failed.";
    });

    if (auto *deck = dynamic_cast<ZoneStack_Deck *>(z)) {
      push("draw() P1 hand", [this, deck] {
        return deck->draw(field_.handZones[1]) ? "draw OK." : "draw failed.";
      });
      push("mill(1) P1 GY", [this, deck] {
        return deck->mill(1, field_.graveyardZones[1]) ? "mill OK."
                                                       : "mill failed.";
      });
    }

    if (auto *zs = dynamic_cast<ZoneStack *>(z)) {
      push("peek(-1) top", [zs]() -> std::string {
        Card *c = zs->peek(-1);
        return c ? "peek(-1) " + c->name : "peek(-1) null.";
      });
      push("peek(0) bottom", [zs]() -> std::string {
        Card *c = zs->peek(0);
        return c ? "peek(0) " + c->name : "peek(0) null.";
      });
      push("findAll(monster)", [zs]() -> std::string {
        auto v = zs->findAll([](const Card *c) { return c->isMonster(); });
        return "findAll(monster) " + std::to_string(v.size()) + " cards.";
      });
      push("shuffle()", [zs]() -> std::string {
        zs->shuffle();
        return "shuffle OK.";
      });
      push("clear()", [zs]() -> std::string {
        zs->clear();
        return "clear OK.";
      });
    }

    push("clearField()", [this]() -> std::string {
      field_.clearField();
      return "clearField OK.";
    });
    push("countMonsters P1", [this]() -> std::string {
      return "countMonsters(P1) " + std::to_string(field_.countMonsters(1));
    });
    push("countMonsters P2", [this]() -> std::string {
      return "countMonsters(P2) " + std::to_string(field_.countMonsters(0));
    });
    push("firstEmptyMon P1", [this]() -> std::string {
      return "firstEmptyMonster(P1) " +
             std::to_string(field_.firstEmptyMonsterZone(1));
    });
    push("firstEmptyMon P2", [this]() -> std::string {
      return "firstEmptyMonster(P2) " +
             std::to_string(field_.firstEmptyMonsterZone(0));
    });
    push("firstEmptyST P1", [this]() -> std::string {
      return "firstEmptySpellTrap(P1) " +
             std::to_string(field_.firstEmptySpellTrapZone(1));
    });

    for (auto &a : actions_)
      actionLabels_.push_back(a.label);
  }

  Card *nextPoolCard() {
    for (int i = 0; i < (int)pool_.size(); ++i) {
      Card *c = &pool_[poolIdx_];
      poolIdx_ = (poolIdx_ + 1) % (int)pool_.size();
      if (!inAnyZone(c))
        return c;
    }
    return nullptr;
  }

  bool inAnyZone(Card *c) const {
    for (int p = 0; p < 2; ++p) {
      for (int i = 0; i < 5; ++i)
        if (field_.monsterZones[p][i].contains(c) ||
            field_.spellTrapZones[p][i].contains(c))
          return true;
      if (field_.graveyardZones[p].contains(c) ||
          field_.deckZones[p].contains(c) ||
          field_.extraDeckZones[p].contains(c) ||
          field_.fieldZones[p].contains(c) ||
          field_.banishedZones[p].contains(c) ||
          field_.handZones[p].contains(c))
        return true;
    }
    return false;
  }
};

} // namespace openjoey::ui
