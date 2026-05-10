#pragma once
#include "card/Card.hpp"
#include "game/zone/Field.hpp"
#include "game/zone/Zone.hpp"
#include "ui/AppScreen.hpp"
#include "ui/CardImageCache.hpp"
#include "ui/StyleSheet.hpp"
#include "ui/screens/widgets/ZoneCell.hpp"
#include "ui/screens/widgets/ZoneInfoPanel.hpp"
#include <filesystem>
#include <functional>
#include <raylib.h>
#include <string>
#include <vector>

namespace openjoey::ui {
using namespace openjoey::zone;

class DuelScreen {
public:
  DuelScreen() {
    loadCardBack();
    buildPool();
    buildGrid();
    seedField();
    rebuildActions();
  }

  ~DuelScreen() {
    if (cardBack_.id)
      UnloadTexture(cardBack_);
  }

  void Update() {
    imageCache_.PollAndLoad();
    handleInput();
  }

  void Draw() const {
    ClearBackground(COLOR_BG_DARK);

    int leftW = _SW * 30 / 100;
    int rightW = _SW * 15 / 100;
    int centerW = _SW - leftW - rightW;
    int headerH = HEADER_HEIGHT;
    int footerH = int(0.03f * _SH);
    int fieldH = _SH - headerH - footerH;

    drawHeader(0, 0, _SW, headerH);
    drawPreviewPanel(0, headerH, leftW, fieldH);
    drawField(leftW, headerH, centerW, fieldH);
    ZoneInfoPanel::Draw({(float)(leftW + centerW), (float)headerH,
                         (float)rightW, (float)fieldH},
                        cursorZone(), cursorLabel(), actionLabels_,
                        actionCursor_, lastResult_, selectedZone_ != nullptr,
                        imageCache_, cardBack_.id ? &cardBack_ : nullptr);
    drawFooter(0, _SH - footerH, _SW, footerH);
  }

  AppScreen NextScreen() const { return next_; }

private:
  openjoey::zone::Field field_;
  std::vector<openjoey::Card> pool_;
  mutable CardImageCache imageCache_;
  Texture2D cardBack_ = {};
  int poolIdx_ = 0;

  // ROWS=6: row0=P2hand  row1=P2-monster+BAN/GY/FLD  row2=P2-ST+ED/DK
  //         row3=P1-ST+DK/ED  row4=P1-monster+FLD/GY/BAN  row5=P1hand
  // COLS=9: [0][1] = left side  [2..6] = 5 main zones  [7][8] = right side
  static constexpr int ROWS = 6, COLS = 9;
  IZone *grid_[ROWS][COLS] = {};
  const char *labels_[ROWS][COLS] = {};
  int cursorRow_ = 4, cursorCol_ = 4; // start on P1 M1[2] (Blue-Eyes)
  IZone *selectedZone_ = nullptr;
  int handCursor_ = 0;
  AppScreen next_ = AppScreen::Duel;

  struct Action {
    std::string label;
    std::function<std::string()> invoke;
  };
  std::vector<Action> actions_;
  std::vector<std::string> actionLabels_;
  int actionCursor_ = 0;
  std::string lastResult_;

  static constexpr const char *kST[2][5] = {
      {"ST2-0", "ST2-1", "ST2-2", "ST2-3", "ST2-4"},
      {"ST1-0", "ST1-1", "ST1-2", "ST1-3", "ST1-4"},
  };
  static constexpr const char *kM[2][5] = {
      {"M2-0", "M2-1", "M2-2", "M2-3", "M2-4"},
      {"M1-0", "M1-1", "M1-2", "M1-3", "M1-4"},
  };

  // ─────────────────────────────────────── setup
  void loadCardBack() {
    for (auto &p :
         {"data/assets/card_back.jpg", "../data/assets/card_back.jpg"}) {
      if (std::filesystem::exists(p)) {
        cardBack_ = LoadTexture(p);
        if (cardBack_.id)
          break;
      }
    }
  }

  static openjoey::Card makeCard(const char *name, uint32_t num,
                                 const char *desc, CardType t, int atk, int def,
                                 int lvl) {
    openjoey::Card c;
    c.name = name;
    c.cardNumber = num;
    c.description = desc;
    c.type = t;
    c.atk = atk;
    c.def = def;
    c.level = lvl;
    return c;
  }

  void buildPool() {
    pool_ = {
        makeCard("Blue-Eyes", 89631139, "3000 ATK dragon.", CardType::Monster,
                 3000, 2500, 8),
        makeCard("Dk Magician", 46986414, "2500 ATK spellcaster.",
                 CardType::Monster, 2500, 2100, 7),
        makeCard("Kuriboh", 40640057, "Reduce damage to 0.", CardType::Monster,
                 300, 200, 1),
        makeCard("Raigeki", 12580477, "Destroy all opp mons.", CardType::Spell,
                 0, 0, 0),
        makeCard("Reborn", 83764719, "Special Summon 1 mon.", CardType::Spell,
                 0, 0, 0),
        makeCard("Mirror Force", 44095762, "Destroy ATK monsters.",
                 CardType::Trap, 0, 0, 0),
    };
  }

  void buildGrid() {
    // Row 1 — P2: [BAN2][GY2] | M2[4..0] | [FLD2]
    grid_[1][0] = &field_.banishedZones[0];
    labels_[1][0] = "BAN2";
    grid_[1][1] = &field_.graveyardZones[0];
    labels_[1][1] = "GY2";
    for (int i = 0; i < 5; ++i) {
      grid_[1][2 + i] = &field_.monsterZones[0][4 - i];
      labels_[1][2 + i] = kM[0][4 - i];
    }
    grid_[1][7] = &field_.fieldZones[0];
    labels_[1][7] = "FLD2";
    // col 8 = null

    // Row 2 — P2: [ED2] | ST2[4..0] | [DK2]
    // col 0 = null (ED spans full left side width via cellRect)
    grid_[2][1] = &field_.extraDeckZones[0];
    labels_[2][1] = "ED2";
    for (int i = 0; i < 5; ++i) {
      grid_[2][2 + i] = &field_.spellTrapZones[0][4 - i];
      labels_[2][2 + i] = kST[0][4 - i];
    }
    grid_[2][7] = &field_.deckZones[0];
    labels_[2][7] = "DK2";
    // col 8 = null

    // Row 3 — P1: [DK1] | ST1[0..4] | [ED1]
    // col 0 = null
    grid_[3][1] = &field_.deckZones[1];
    labels_[3][1] = "DK1";
    for (int i = 0; i < 5; ++i) {
      grid_[3][2 + i] = &field_.spellTrapZones[1][i];
      labels_[3][2 + i] = kST[1][i];
    }
    grid_[3][7] = &field_.extraDeckZones[1];
    labels_[3][7] = "ED1";
    // col 8 = null

    // Row 4 — P1: [FLD1] | M1[0..4] | [GY1][BAN1]
    grid_[4][0] = &field_.fieldZones[1];
    labels_[4][0] = "FLD1";
    // col 1 = null (FLD spans full left side width via cellRect)
    for (int i = 0; i < 5; ++i) {
      grid_[4][2 + i] = &field_.monsterZones[1][i];
      labels_[4][2 + i] = kM[1][i];
    }
    grid_[4][7] = &field_.graveyardZones[1];
    labels_[4][7] = "GY1";
    grid_[4][8] = &field_.banishedZones[1];
    labels_[4][8] = "BAN1";
    // Rows 0 and 5 are hand rows — handled separately.
  }

  void seedField() {
    for (auto &c : pool_) {
      c.owner = 1;
      c.controller = 1;
    }
    pool_[1].owner = 0;
    pool_[1].controller = 0;

    for (int i = 2; i < (int)pool_.size(); ++i)
      field_.deckZones[1].put(&pool_[i]);

    field_.monsterZones[1][2].put(&pool_[0]);
    field_.monsterZones[1][2].changeVisibility(Visibility::Visible);
    field_.monsterZones[1][2].changeOrientation(Orientation::Vertical);

    field_.monsterZones[0][2].put(&pool_[1]);
    field_.monsterZones[0][2].changeVisibility(Visibility::Visible);
    field_.monsterZones[0][2].changeOrientation(Orientation::Vertical);
  }

  // ─────────────────────────────────────── input
  void handleInput() {
    if (IsKeyPressed(KEY_ESCAPE)) {
      if (selectedZone_) {
        selectedZone_ = nullptr;
        lastResult_ = "Deselected.";
      } else
        next_ = AppScreen::MainMenu;
      return;
    }
    if (IsKeyPressed(KEY_ENTER)) {
      handleEnter();
      return;
    }

    if (IsKeyPressed(KEY_TAB)) {
      if (!actions_.empty())
        actionCursor_ = (actionCursor_ + 1) % (int)actions_.size();
      return;
    }

    for (int k = KEY_ONE; k <= KEY_NINE; ++k) {
      if (IsKeyPressed(k)) {
        int idx = k - KEY_ONE;
        if (idx < (int)actions_.size()) {
          lastResult_ = actions_[idx].invoke();
          rebuildActions();
        }
        return;
      }
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
      moveCursor(dr, dc);
      rebuildActions();
    }
  }

  void handleEnter() {
    IZone *z = cursorZone();
    if (!z)
      return;
    if (!selectedZone_) {
      if (!z->isEmpty()) {
        selectedZone_ = z;
        lastResult_ = "Source selected.";
      } else
        lastResult_ = "Zone empty — nothing to pick.";
    } else {
      if (z == selectedZone_) {
        selectedZone_ = nullptr;
        lastResult_ = "Deselected.";
        return;
      }
      lastResult_ = selectedZone_->moveTo(*z) ? "Moved OK." : "Move failed.";
      selectedZone_ = nullptr;
      rebuildActions();
    }
  }

  void moveCursor(int dr, int dc) {
    bool inHand = (cursorRow_ == 0 || cursorRow_ == 5);
    if (inHand) {
      if (dc) {
        int p = (cursorRow_ == 0) ? 0 : 1;
        int cnt = field_.handZones[p].count();
        if (cnt > 0)
          handCursor_ = (handCursor_ + dc + cnt) % cnt;
        return;
      }
      if (dr) {
        int nr = std::clamp(cursorRow_ + dr, 0, ROWS - 1);
        if (nr != cursorRow_) {
          cursorRow_ = nr;
          if (nr > 0 && nr < 5)
            snapCol();
        }
      }
      return;
    }
    if (dr) {
      int nr = cursorRow_ + dr;
      if (nr < 0 || nr >= ROWS)
        return;
      if (nr == 0 || nr == 5) {
        cursorRow_ = nr;
        handCursor_ = 0;
        return;
      }
      int nc = cursorCol_;
      if (!grid_[nr][nc]) {
        for (int d = 1; d < COLS; ++d) {
          if (nc - d >= 0 && grid_[nr][nc - d]) {
            nc = nc - d;
            break;
          }
          if (nc + d < COLS && grid_[nr][nc + d]) {
            nc = nc + d;
            break;
          }
        }
      }
      if (grid_[nr][nc]) {
        cursorRow_ = nr;
        cursorCol_ = nc;
      }
      return;
    }
    if (dc) {
      int nc = cursorCol_ + dc;
      while (nc >= 0 && nc < COLS) {
        if (grid_[cursorRow_][nc]) {
          cursorCol_ = nc;
          return;
        }
        nc += dc;
      }
    }
  }

  void snapCol() {
    if (grid_[cursorRow_][cursorCol_])
      return;
    for (int d = 1; d < COLS; ++d) {
      if (cursorCol_ - d >= 0 && grid_[cursorRow_][cursorCol_ - d]) {
        cursorCol_ -= d;
        return;
      }
      if (cursorCol_ + d < COLS && grid_[cursorRow_][cursorCol_ + d]) {
        cursorCol_ += d;
        return;
      }
    }
  }

  // ─────────────────────────────────────── actions
  void rebuildActions() {
    actions_.clear();
    actionLabels_.clear();
    actionCursor_ = 0;
    IZone *z = cursorZone();
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
                   : "changeOrientation → false.";
      });
      push("setDEF  [Horizontal]", [zm] {
        return zm->changeOrientation(Orientation::Horizontal)
                   ? "DEF OK."
                   : "changeOrientation → false.";
      });
      push("vis: Visible  (both)", [zm] {
        return zm->changeVisibility(Visibility::Visible)
                   ? "Visible OK."
                   : "changeVisibility → false.";
      });
      push("vis: Limited  (owner)", [zm] {
        return zm->changeVisibility(Visibility::Limited)
                   ? "Limited OK."
                   : "changeVisibility → false.";
      });
      push("vis: Restricted (FD)", [zm] {
        return zm->changeVisibility(Visibility::Restricted)
                   ? "Restricted OK."
                   : "changeVisibility → false.";
      });
      push("flip()", [zm] {
        return zm->flip() ? "flip OK."
                          : "flip → false (need Vertical+Limited).";
      });
    }

    if (auto *deck = dynamic_cast<ZoneStack_Deck *>(z)) {
      push("draw() → P1 hand", [this, deck] {
        return deck->draw(field_.handZones[1]) ? "draw OK." : "draw failed.";
      });
      push("mill(1) → P1 GY", [this, deck] {
        return deck->mill(1, field_.graveyardZones[1]) ? "mill OK."
                                                       : "mill failed.";
      });
      push("shuffle()", [deck] {
        deck->shuffle();
        return "shuffle OK.";
      });
    }
    if (auto *zs = dynamic_cast<ZoneStack *>(z))
      push("clear()", [zs] {
        zs->clear();
        return "clear OK.";
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
    for (int r = 1; r <= 4; ++r)
      for (int col = 0; col < COLS; ++col)
        if (grid_[r][col] && grid_[r][col]->contains(c))
          return true;
    return field_.handZones[0].contains(c) || field_.handZones[1].contains(c);
  }

  // ─────────────────────────────────────── draw
  void drawHeader(int x, int y, int w, int h) const {
    DrawRectangle(x, y, w, h, COLOR_HEADER_BG);
    DrawLine(x, y + h - 1, x + w, y + h - 1, Color{50, 50, 80, 255});
    int fs = FONT_SCREEN_TITLE;
    const char *mode = selectedZone_ ? "SELECT DEST  [ESC=cancel]"
                                     : "NAVIGATE  [ENTER=pick source]";
    DrawText(("Duel Field  —  " + std::string(mode)).c_str(),
             x + HEADER_TITLE_X, y + (h - fs) / 2, fs, YELLOW);
    const char *cl = cursorLabel();
    int tw = MeasureText(cl, fs);
    DrawText(cl, x + w - tw - HEADER_TITLE_X, y + (h - fs) / 2, fs,
             COLOR_STAT_TEXT);
  }

  void drawFooter(int x, int y, int w, int h) const {
    DrawRectangle(x, y, w, h, COLOR_FOOTER_BG);
    DrawLine(x, y, x + w, y, Color{50, 50, 80, 255});
    int fs = FONT_HELP_TEXT;
    DrawText("Arrows:navigate  Enter:select/move  1-9:action  Tab:cycle action "
             " Esc:back",
             x + MAIN_PAD_X, y + (h - fs) / 2, fs, COLOR_STAT_TEXT);
  }

  void drawPreviewPanel(int x, int y, int w, int h) const {
    DrawRectangle(x, y, w, h, COLOR_BG_MAIN);
    DrawLine(x + w - 1, y, x + w - 1, y + h, Color{50, 50, 80, 255});

    int pad = PREVIEW_PAD_X;
    int cy = y + pad;
    int fs = FONT_CARD_STAT;
    int fsT = FONT_CARD_NAME;

    DrawText("Preview", x + pad, cy, FONT_PANEL_TITLE, COLOR_STAT_TEXT);
    cy += FONT_PANEL_TITLE + pad / 2;

    float aspect = 59.f / 86.f;
    int cardW = w - pad * 2;
    int cardH = (int)(cardW / aspect);
    if (cardH > h * 45 / 100) {
      cardH = h * 45 / 100;
      cardW = (int)(cardH * aspect);
    }
    int cardX = x + (w - cardW) / 2;
    Rectangle cardR = {(float)cardX, (float)cy, (float)cardW, (float)cardH};

    IZone *z = cursorZone();
    Card *top = nullptr;
    bool fd = false;
    if (z && !z->isEmpty()) {
      if (auto *zm = dynamic_cast<Zone_Monster *>(z)) {
        top = zm->peek();
        fd = zm->visibility() == Visibility::Restricted;
      } else if (auto *zs = dynamic_cast<ZoneStack *>(z))
        top = zs->peek(-1);
      else if (auto *zn = dynamic_cast<Zone *>(z))
        top = zn->peek();
    }

    const Texture2D *cb = cardBack_.id ? &cardBack_ : nullptr;
    if (fd) {
      if (cb)
        DrawTexturePro(*cb, {0, 0, (float)cb->width, (float)cb->height}, cardR,
                       {0, 0}, 0.f, WHITE);
      else
        DrawRectangleRec(cardR, Color{8, 6, 42, 255});
      DrawRectangleLinesEx(cardR, 1.5f, Color{210, 170, 40, 255});
    } else if (top) {
      const Texture2D *tex = imageCache_.Get(*top);
      if (tex && tex->id) {
        DrawTexturePro(*tex, {0, 0, (float)tex->width, (float)tex->height},
                       cardR, {0, 0}, 0.f, WHITE);
      } else {
        Color fc = top->isMonster() ? Color{88, 60, 60, 255}
                   : top->isSpell() ? Color{56, 90, 72, 255}
                                    : Color{88, 56, 92, 255};
        DrawRectangleRec(cardR, fc);
      }
      DrawRectangleLinesEx(cardR, 1.2f, Color{200, 180, 100, 255});
    } else {
      DrawRectangleRec(cardR, COLOR_BG_MAIN);
      DrawRectangleLinesEx(cardR, 1.f, Color{50, 50, 70, 255});
    }
    cy += cardH + pad;

    if (top && !fd) {
      DrawText(top->name.c_str(), x + pad, cy, fsT, WHITE);
      cy += fsT + 3;
      DrawText(top->cardTypeTag().c_str(), x + pad, cy, fs,
               top->isMonster() ? COLOR_MONSTER_STAT
               : top->isSpell() ? COLOR_SPELL_STAT
                                : COLOR_TRAP_STAT);
      cy += fs + 3;
      if (top->isMonster()) {
        DrawText(top->statLine().c_str(), x + pad, cy, fs, COLOR_STAT_TEXT);
        cy += fs + 4;
      }
      const std::string &desc = top->description;
      int lineFs = FONT_HELP_TEXT;
      int lineH2 = lineFs + 3;
      int maxPx = w - pad * 2;
      size_t pos = 0;
      while (pos < desc.size() && cy + lineH2 < y + h - pad) {
        size_t end = pos;
        std::string line;
        while (end < desc.size()) {
          size_t nxt = desc.find(' ', end + 1);
          if (nxt == std::string::npos)
            nxt = desc.size();
          std::string trial = desc.substr(pos, nxt - pos);
          if (MeasureText(trial.c_str(), lineFs) > maxPx)
            break;
          line = trial;
          end = nxt;
        }
        if (end == pos) {
          end = pos + 1;
          line = desc.substr(pos, 1);
        }
        DrawText(line.c_str(), x + pad, cy, lineFs, COLOR_DESC_TEXT);
        cy += lineH2;
        pos = (end < desc.size() && desc[end] == ' ') ? end + 1 : end;
      }
    }
  }

  void drawField(int fx, int fy, int fw, int fh) const {
    DrawRectangle(fx, fy, fw, fh, Color{14, 20, 14, 255}); // felt-green mat

    int gapX = fw * 5 / 1000;
    int gapY = fh * 6 / 1000;
    int sideW = fw * 9 / 100;
    int mainW = fw - sideW * 2;
    int handH = fh * 11 / 100;

    // 4 field rows: P2support, P2mon, P1mon, P1support
    int zoneH = (fh - handH * 2 - gapY * 5) / 4;
    int zoneW = (mainW - gapX * 6) / 5;
    if (zoneW > zoneH * 85 / 100)
      zoneW = zoneH * 85 / 100;

    int boardX = fx + sideW;

    int rowY[4];
    rowY[0] = fy + handH + gapY;
    rowY[1] = rowY[0] + zoneH + gapY;
    rowY[2] = rowY[1] + zoneH + gapY;
    rowY[3] = rowY[2] + zoneH + gapY;

    drawOppHand(fx, fy, fw, handH);

    // Draw all field rows 1-4; null cells get a subtle empty-slot background
    // so monster rows have no visual gaps on the sides.
    for (int gridRow = 1; gridRow <= 4; ++gridRow) {
      DrawRectangle(fx, rowY[gridRow - 1], fw, zoneH,
                    Fade(COLOR_BG_MAIN, 0.85f));

      for (int col = 0; col < COLS; ++col) {
        Rectangle r = cellRect(gridRow, col, fx, fw, boardX, zoneW, zoneH, gapX,
                               sideW, rowY);
        bool isCur = (cursorRow_ == gridRow && cursorCol_ == col);
        bool isSel = (grid_[gridRow][col] == selectedZone_);
        const Texture2D *cb = cardBack_.id ? &cardBack_ : nullptr;
        if (grid_[gridRow][col]) {
          ZoneCell::Draw(r, grid_[gridRow][col], labels_[gridRow][col], isCur,
                         isSel, imageCache_, cb);
        }
      }
    }

    // Center divider — between P2 ST row and P1 ST row
    int divY = rowY[1] + zoneH + gapY / 2;
    DrawLineEx({(float)(fx + 2), (float)divY},
               {(float)(fx + fw - 2), (float)divY}, 2.f,
               Color{80, 80, 110, 220});

    drawOwnHand(fx, fy + fh - handH, fw, handH);
  }

  // Returns screen rect for every cell (null or not) — drives both ZoneCell and
  // empty-slot draws. Layout per row:
  //   row1 (P2 monster): col0=BAN2(half-L)  col1=GY2(half-L)  cols2-6=M×5
  //   col7=FLD2(full-R)  col8=empty row2 (P2 ST):      col0=empty
  //   col1=ED2(full-L)  cols2-6=ST×5 col7=DK2(full-R)   col8=empty row3 (P1
  //   ST):      col0=empty          col1=DK1(full-L)  cols2-6=ST×5
  //   col7=ED1(full-R)   col8=empty row4 (P1 monster): col0=FLD1(full-L)
  //   col1=empty        cols2-6=M×5  col7=GY1(half-R)   col8=BAN1(half-R)
  Rectangle cellRect(int row, int col, int fx, int fw, int boardX, int zoneW,
                     int zoneH, int gapX, int sideW, const int rowY[4]) const {
    int ry = rowY[row - 1];
    int half = sideW / 2;

    // ── left side (cols 0, 1)
    if (col == 0) {
      if (row == 1)
        return {(float)fx, (float)ry, (float)(half - 1),
                (float)zoneH}; // BAN2 half
      if (row == 4)
        return {(float)fx, (float)ry, (float)(sideW - 1),
                (float)zoneH}; // FLD1 full
      return {(float)fx, (float)ry, (float)(half - 1),
              (float)zoneH}; // empty slot
    }
    if (col == 1) {
      if (row == 1)
        return {(float)(fx + half), (float)ry, (float)(half - 1),
                (float)zoneH}; // GY2 half
      if (row == 2)
        return {(float)fx, (float)ry, (float)(sideW - 1),
                (float)zoneH}; // ED2 full
      if (row == 3)
        return {(float)fx, (float)ry, (float)(sideW - 1),
                (float)zoneH}; // DK1 full
      return {(float)(fx + half), (float)ry, (float)(half - 1),
              (float)zoneH}; // empty slot
    }

    // ── center (cols 2-6)
    if (col >= 2 && col <= 6) {
      int cx = boardX + gapX + (col - 2) * (zoneW + gapX);
      return {(float)cx, (float)ry, (float)zoneW, (float)zoneH};
    }

    // ── right side (cols 7, 8)
    if (col == 7) {
      if (row == 4)
        return {(float)(fx + fw - sideW), (float)ry, (float)(half - 1),
                (float)zoneH}; // GY1 half
      return {(float)(fx + fw - sideW), (float)ry, (float)(sideW - 1),
              (float)zoneH}; // full
    }
    if (col == 8) {
      // only row4 col8 = BAN1 half; everything else is empty slot
      return {(float)(fx + fw - half), (float)ry, (float)(half - 1),
              (float)zoneH};
    }

    // fallback
    int cellW = fw / COLS;
    return {(float)(fx + col * cellW), (float)ry, (float)(cellW - 2),
            (float)zoneH};
  }

  void drawOppHand(int fx, int fy, int fw, int handH) const {
    DrawRectangle(fx, fy, fw, handH, COLOR_BG_DARK);
    DrawLine(fx, fy + handH - 1, fx + fw, fy + handH - 1,
             Color{40, 40, 65, 180});
    int cnt = field_.handZones[0].count();
    int fs = FONT_CARD_STAT;
    DrawText(TextFormat("P2 Hand: %d", cnt), fx + MAIN_PAD_X,
             fy + (handH - fs) / 2, fs, COLOR_STAT_TEXT);
    if (cnt == 0)
      return;
    int cw = (int)(handH * 0.75f * (59.f / 86.f));
    int ch = (int)(handH * 0.75f);
    int totalW = cnt * (cw + 3) - 3;
    int startX = fx + (fw - totalW) / 2;
    const Texture2D *cb = cardBack_.id ? &cardBack_ : nullptr;
    for (int i = 0; i < cnt; ++i) {
      int cx = startX + i * (cw + 3);
      int cy2 = fy + (handH - ch) / 2;
      if (cb)
        DrawTexturePro(*cb, {0, 0, (float)cb->width, (float)cb->height},
                       {(float)cx, (float)cy2, (float)cw, (float)ch}, {0, 0},
                       0.f, WHITE);
      else {
        DrawRectangle(cx, cy2, cw, ch, COLOR_BG_DARK);
        DrawRectangleLines(cx, cy2, cw, ch, Color{210, 170, 40, 255});
      }
    }
  }

  void drawOwnHand(int fx, int fy, int fw, int handH) const {
    DrawRectangle(fx, fy, fw, handH, COLOR_BG_DARK);
    DrawLine(fx, fy, fx + fw, fy, Color{40, 40, 65, 180});
    auto &hand = const_cast<ZoneStack_Hand &>(field_.handZones[1]);
    int cnt = hand.count();
    int fs = FONT_CARD_STAT;
    DrawText(TextFormat("P1 Hand: %d", cnt), fx + MAIN_PAD_X, fy + 3, fs,
             COLOR_STAT_TEXT);
    if (cnt == 0) {
      DrawText("(empty)", fx + fw / 2, fy + (handH - fs) / 2, fs, DARKGRAY);
      return;
    }
    int cw = (int)(handH * 0.85f * (59.f / 86.f));
    int ch = (int)(handH * 0.85f);
    int totalW = cnt * (cw + 4) - 4;
    int startX = fx + (fw - totalW) / 2;
    for (int i = 0; i < cnt; ++i) {
      Card *c = hand.peek(i);
      if (!c)
        continue;
      int cx = startX + i * (cw + 4);
      int cy2 = fy + (handH - ch) - 4;
      bool cur = (cursorRow_ == 5 && handCursor_ == i);
      bool sel = selectedZone_ == &field_.handZones[1];
      const Texture2D *tex = imageCache_.Get(*c);
      Rectangle cr = {(float)cx, (float)cy2, (float)cw, (float)ch};
      if (tex && tex->id)
        DrawTexturePro(*tex, {0, 0, (float)tex->width, (float)tex->height}, cr,
                       {0, 0}, 0.f, WHITE);
      else {
        Color fc = c->isMonster() ? COLOR_MONSTER_STAT
                   : c->isSpell() ? COLOR_SPELL_STAT
                                  : COLOR_TRAP_STAT;
        DrawRectangleRec(cr, Fade(fc, 0.6f));
        DrawText(c->name.substr(0, 6).c_str(), (int)cx + 2, (int)cy2 + 2,
                 FONT_HELP_TEXT, WHITE);
      }
      float thick = (cur || sel) ? 2.5f : 1.f;
      Color border = cur ? YELLOW : sel ? GREEN : Color{180, 180, 210, 200};
      DrawRectangleLinesEx(cr, thick, border);
    }
  }

  // ─────────────────────────────────────── helpers
  IZone *cursorZone() const {
    auto &f = const_cast<openjoey::zone::Field &>(field_);
    if (cursorRow_ == 0)
      return &f.handZones[0];
    if (cursorRow_ == 5)
      return &f.handZones[1];
    return grid_[cursorRow_][cursorCol_];
  }

  const char *cursorLabel() const {
    if (cursorRow_ == 0)
      return "P2 Hand";
    if (cursorRow_ == 5)
      return "P1 Hand";
    IZone *z = grid_[cursorRow_][cursorCol_];
    return z ? labels_[cursorRow_][cursorCol_] : "---";
  }
};

} // namespace openjoey::ui
