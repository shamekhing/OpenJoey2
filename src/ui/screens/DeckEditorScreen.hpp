#pragma once
#include "card/Card.hpp"
#include "card/CardDatabase.hpp"
#include "ui/AppScreen.hpp"
#include "ui/CardImageCache.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <raylib.h>
#include <string>
#include <vector>

#define isInputChar(c) ((c >= 32 && c < 127) || c == '\b')
using cmpFunArrPointer = bool (*)(const openjoey::Card &,
                                  const openjoey::Card &);

namespace openjoey::ui {

enum class FucusedWidget { Pool, Searchbar, Deck, None };

enum class DeckSortMode {
  Type,
  NameDesc,
  NameAsc,
  LevelDesc,
  LevelAsc,
  AtkDesc,
  AtkAsc,
  DefDesc,
  DefAsc,
  COUNT
};
enum class DeckTypeFilter { All, Monster, Spell, Trap, COUNT };

class DeckEditorScreen {
public:
  static constexpr int kMinDeckSize = 40;
  static constexpr int kMaxDeckSize = 60;
  static constexpr int kMaxCopies = 3;

  DeckEditorScreen(const openjoey::CardDatabase &db) : db_(db) {
    rebuildPool();
  }

  void Update();
  void Draw() const;
  AppScreen NextScreen() const { return next_; }
  bool DeckReady() const { return deckReady_; }
  const std::vector<openjoey::Card> &GetDeck() const { return deck_; }

  bool SaveDeck(const std::string &name) const;
  bool LoadDeck(const std::string &name);

private:
  const openjoey::CardDatabase &db_;
  std::vector<openjoey::Card> pool_;
  std::vector<openjoey::Card> deck_;

  bool focusPool_ = true;
  int poolCursor_ = 0;
  int deckCursor_ = 0;

  bool inSearch_ = false;
  std::string searchQuery_;

  DeckSortMode sortMode_ = DeckSortMode::Type;
  DeckTypeFilter typeFilter_ = DeckTypeFilter::All;

  AppScreen next_ = AppScreen::DeckEditor;
  bool deckReady_ = false;
  std::string statusMsg_;

  mutable CardImageCache imgCache_;

  void rebuildPool();
  std::vector<const openjoey::Card *> filteredPool() const;
  int countInDeck(uint32_t id) const;

  void drawPoolPanel(int x, int y, int w, int h) const;
  void drawPreviewPanel(int x, int y, int w, int h) const;
  void drawDeckPanel(int x, int y, int w, int h) const;
  void drawDeckStats(int x, int y, int w) const;
  void drawProgressBar(int x, int y, int w, int h, float frac) const;

  static const char *sortModeLabel(DeckSortMode m);
  static const char *typeFilterLabel(DeckTypeFilter f);
};

// ── File-local helpers

Color cardTypeColor(const openjoey::Card &c) {
  return c.isMonster() ? MAROON : (c.isSpell() ? GREEN : PINK);
}

// ── Pool rebuild

inline void DeckEditorScreen::rebuildPool() {
  pool_.clear();
  for (const auto &c : db_.GetAllCards())
    pool_.push_back(c);

  static constexpr std::pair<cmpFunArrPointer, bool> kSort[] = {
      {openjoey::Card::sortByCardType, false}, // Type
      {openjoey::Card::sortByName, false},     // NameDesc
      {openjoey::Card::sortByName, true},      // NameAsc
      {openjoey::Card::sortByLevel, false},    // LevelDesc
      {openjoey::Card::sortByLevel, true},     // LevelAsc
      {openjoey::Card::sortByAtk, false},      // AtkDesc
      {openjoey::Card::sortByAtk, true},       // AtkAsc
      {openjoey::Card::sortByDef, false},      // DefDesc
      {openjoey::Card::sortByDef, true},       // DefAsc
  };
  auto [cmp, rev] = kSort[(int)sortMode_];
  std::sort(pool_.begin(), pool_.end(), cmp);
  if (rev)
    std::reverse(pool_.begin(), pool_.end());
}

// ── Static label methods
// ──────────────────────────────────────────────────────

inline const char *DeckEditorScreen::sortModeLabel(DeckSortMode m) {
  switch (m) {
  case DeckSortMode::Type:
    return "Type";
  case DeckSortMode::NameDesc:
    return "Name (A-Z)";
  case DeckSortMode::NameAsc:
    return "Name (Z-A)";
  case DeckSortMode::LevelDesc:
    return "Level (desc)";
  case DeckSortMode::LevelAsc:
    return "Level (asc)";
  case DeckSortMode::AtkDesc:
    return "ATK (desc)";
  case DeckSortMode::AtkAsc:
    return "ATK (asc)";
  case DeckSortMode::DefDesc:
    return "DEF (desc)";
  case DeckSortMode::DefAsc:
    return "DEF (asc)";
  default:
    return "?";
  }
}

inline const char *DeckEditorScreen::typeFilterLabel(DeckTypeFilter f) {
  switch (f) {
  case DeckTypeFilter::All:
    return "All";
  case DeckTypeFilter::Monster:
    return "Monster";
  case DeckTypeFilter::Spell:
    return "Spell";
  case DeckTypeFilter::Trap:
    return "Trap";
  default:
    return "?";
  }
}

// ── Filtered pool

inline std::vector<const openjoey::Card *>
DeckEditorScreen::filteredPool() const {
  std::vector<const openjoey::Card *> out;
  std::string q = searchQuery_;
  std::transform(q.begin(), q.end(), q.begin(), ::tolower);

  for (const auto &c : pool_) {
    if (typeFilter_ == DeckTypeFilter::Monster && !c.isMonster())
      continue;
    if (typeFilter_ == DeckTypeFilter::Spell && !c.isSpell())
      continue;
    if (typeFilter_ == DeckTypeFilter::Trap && !c.isTrap())
      continue;
    if (!q.empty()) {
      std::string lo = c.name;
      std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower);
      if (lo.find(q) == std::string::npos)
        continue;
    }
    out.push_back(&c);
  }
  return out;
}

inline int DeckEditorScreen::countInDeck(uint32_t id) const {
  int n = 0;
  for (const auto &c : deck_)
    if (c.cardNumber == id)
      ++n;
  return n;
}

// ── Update
// ────────────────────────────────────────────────────────────────────

inline void DeckEditorScreen::Update() {
  imgCache_.PollAndLoad();

  if (inSearch_) {
    int ch = GetCharPressed();
    do {

    } while (false);
    while (ch > 0) {
      if (isInputChar(ch))
        searchQuery_ += (char)ch;
      ch = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && !searchQuery_.empty())
      searchQuery_.pop_back();
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) {
      inSearch_ = false;
      poolCursor_ = 0;
    }
    return;
  }

  if (IsKeyPressed(KEY_ESCAPE)) {
    next_ = AppScreen::MainMenu;
  }

  if (IsKeyPressed(KEY_O)) {
    sortMode_ =
        static_cast<DeckSortMode>((static_cast<int>(sortMode_) + 1) %
                                  static_cast<int>(DeckSortMode::COUNT));
    rebuildPool();
    poolCursor_ = 0;
    statusMsg_ = std::string("Sort: ") + sortModeLabel(sortMode_);
  }
  if (IsKeyPressed(KEY_T)) {
    typeFilter_ =
        static_cast<DeckTypeFilter>((static_cast<int>(typeFilter_) + 1) %
                                    static_cast<int>(DeckTypeFilter::COUNT));
    poolCursor_ = 0;
    statusMsg_ = std::string("Filter: ") + typeFilterLabel(typeFilter_);
  }

  auto fp = filteredPool();
  int poolSz = (int)fp.size();
  int deckSz = (int)deck_.size();

  if (focusPool_) {
    if (IsKeyPressed(KEY_DOWN))
      poolCursor_ = std::min(poolCursor_ + 1, std::max(0, poolSz - 1));
    if (IsKeyPressed(KEY_UP))
      poolCursor_ = std::max(poolCursor_ - 1, 0);
    if (IsKeyPressed(KEY_PAGE_DOWN))
      poolCursor_ = std::min(poolCursor_ + 10, std::max(0, poolSz - 1));
    if (IsKeyPressed(KEY_PAGE_UP))
      poolCursor_ = std::max(poolCursor_ - 10, 0);

    if ((IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_A)) && poolSz > 0) {
      const auto &card = *fp[poolCursor_];
      if ((int)deck_.size() < kMaxDeckSize &&
          countInDeck(card.cardNumber) < kMaxCopies) {
        deck_.push_back(card);
        statusMsg_ = "Added: " + card.name;
      } else if (countInDeck(card.cardNumber) >= kMaxCopies) {
        statusMsg_ = "Max 3 copies of " + card.name;
      } else {
        statusMsg_ = "Deck full (60 cards max)";
      }
    }
    if (IsKeyPressed(KEY_Q)) {
      inSearch_ = true;
      searchQuery_.clear();
    }
    if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_RIGHT))
      focusPool_ = false;
  } else {
    if (IsKeyPressed(KEY_DOWN))
      deckCursor_ = std::min(deckCursor_ + 1, deckSz - 1);
    if (IsKeyPressed(KEY_UP))
      deckCursor_ = std::max(deckCursor_ - 1, 0);
    if (IsKeyPressed(KEY_PAGE_DOWN))
      deckCursor_ = std::min(deckCursor_ + 10, deckSz - 1);
    if (IsKeyPressed(KEY_PAGE_UP))
      deckCursor_ = std::max(deckCursor_ - 10, 0);

    if ((IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE) ||
         IsKeyPressed(KEY_D)) &&
        deckSz > 0) {
      statusMsg_ = "Removed: " + deck_[deckCursor_].name;
      deck_.erase(deck_.begin() + deckCursor_);
      if (deckCursor_ >= (int)deck_.size() && deckCursor_ > 0)
        --deckCursor_;
    }
    if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_LEFT))
      focusPool_ = true;
  }

  if (IsKeyPressed(KEY_S)) {
    SaveDeck("default");
    statusMsg_ = "Saved as 'default'";
  }
  if (IsKeyPressed(KEY_L)) {
    statusMsg_ = LoadDeck("default") ? "Loaded 'default'" : "No saved deck";
  }
  if (IsKeyPressed(KEY_C)) {
    deck_.clear();
    deckCursor_ = 0;
    statusMsg_ = "Deck cleared";
  }
  if (IsKeyPressed(KEY_F)) {
    if ((int)deck_.size() >= kMinDeckSize) {
      deckReady_ = true;
      next_ = AppScreen::Duel;
    } else
      statusMsg_ = "Need " + std::to_string(kMinDeckSize - (int)deck_.size()) +
                   " more cards";
  }
  if (IsKeyPressed(KEY_ESCAPE))
    statusMsg_.clear();
}

// ── Draw helpers
// ──────────────────────────────────────────────────────────────

inline void DeckEditorScreen::drawProgressBar(int x, int y, int w, int h,
                                              float frac) const {
  Color fill = (frac >= 1.0f) ? GREEN : YELLOW;
  DrawRectangle(x, y, w, h, {40, 40, 40, 255});
  DrawRectangle(x, y, (int)(w * std::min(frac, 1.0f)), h, fill);
  DrawRectangleLines(x, y, w, h, GRAY);
}

inline void DeckEditorScreen::drawPoolPanel(int x, int y, int w, int h) const {
  Color border = focusPool_ ? YELLOW : DARKGRAY;
  DrawRectangleLines(x, y, w, h, border);
  DrawText("Card Pool", x + 8, y + 6, 16, border);

  auto fp = filteredPool();
  std::string badge = std::string(sortModeLabel(sortMode_)) + "  [" +
                      typeFilterLabel(typeFilter_) + "]  " +
                      std::to_string(fp.size()) + " cards";
  int bw = MeasureText(badge.c_str(), 11);
  DrawText(badge.c_str(), x + w - bw - 6, y + 8, 11, Color{160, 160, 200, 255});

  int searchY = y + 28;
  Color sboxCol = inSearch_ ? YELLOW : GRAY;
  DrawRectangleLines(x + 4, searchY, w - 8, 30, sboxCol);
  std::string sq =
      (inSearch_ ? "/" : "/ ") + searchQuery_ + (inSearch_ ? "_" : "");
  DrawText(sq.c_str(), x + 8, searchY + 2, 24, sboxCol);

  const int itemH = 90;
  const int txW = 58;
  const int txH = itemH - 4;
  const int textX = x + 4 + txW + 6;
  int listY = searchY + 36;
  int maxVis = (y + h - listY - 4) / itemH;
  int scroll = std::max(0, poolCursor_ - maxVis / 2);

  for (int i = 0; i < maxVis && scroll + i < (int)fp.size(); ++i) {
    int idx = scroll + i;
    const auto &c = *fp[idx];
    bool sel = focusPool_ && idx == poolCursor_;
    int copies = countInDeck(c.cardNumber);
    Color typeCol = cardTypeColor(c);
    int iy = listY + i * itemH;

    { // thumbnail
      const Texture2D *tex = imgCache_.Get(c);
      Rectangle dst = {(float)(x + 4), (float)(iy + 2), (float)txW, (float)txH};
      if (tex && tex->id != 0)
        DrawTexturePro(*tex, {0, 0, (float)tex->width, (float)tex->height}, dst,
                       {0, 0}, 0, WHITE);
      else
        DrawRectangle(x + 4, iy + 2, txW, txH,
                      Color{typeCol.r, typeCol.g, typeCol.b, 90});
      DrawRectangleLines(x + 4, iy + 2, txW, txH, typeCol);
    }
    DrawText(c.cardTypeTag().c_str(), textX, iy + 6, 13, typeCol);

    std::string stat = c.shortStat();
    if (!stat.empty())
      DrawText(stat.c_str(), textX, iy + 24, 12, Color{160, 160, 180, 255});

    std::string name = c.name;
    int maxNameW = w - textX - 36;
    while (!name.empty() && MeasureText(name.c_str(), 14) > maxNameW)
      name.pop_back();
    if (name.size() < c.name.size())
      name += "~";
    DrawText(name.c_str(), textX, iy + 44, 14, sel ? YELLOW : WHITE);

    Color cpCol = (copies >= kMaxCopies) ? RED : (copies > 0 ? GREEN : GRAY);
    DrawText((std::to_string(copies) + "/3").c_str(), x + w - 34, iy + 6, 13,
             cpCol);

    if (sel)
      DrawRectangleLines(x + 2, iy, w - 4, itemH, YELLOW);
  }

  if ((int)fp.size() > maxVis) {
    int barH = h - 56 - 4;
    int barX = x + w - 5;
    int barY = listY;
    float frac = (float)scroll / std::max(1, (int)fp.size() - maxVis);
    int thumbH = std::max(8, barH * maxVis / std::max(1, (int)fp.size()));
    int thumbY = barY + (int)(frac * (barH - thumbH));
    DrawRectangle(barX, barY, 3, barH, Color{40, 40, 60, 255});
    DrawRectangle(barX, thumbY, 3, thumbH, Color{160, 160, 200, 200});
  }
}

inline void DeckEditorScreen::drawPreviewPanel(int x, int y, int w,
                                               int h) const {
  DrawRectangleLines(x, y, w, h, DARKGRAY);
  DrawText("Preview", x + 8, y + 6, 14, DARKGRAY);

  auto fp = filteredPool();
  const openjoey::Card *card = nullptr;
  if (focusPool_ && !fp.empty() && poolCursor_ < (int)fp.size())
    card = fp[poolCursor_];
  else if (!focusPool_ && !deck_.empty() && deckCursor_ < (int)deck_.size())
    card = &deck_[deckCursor_];

  if (!card)
    return;

  Color col = cardTypeColor(*card);
  int artX = x + 8, artY = y + 28;
  int artW = w - 16;
  int artH = (int)(artW * 1.45f);
  if (artY + artH > y + h - 80)
    artH = y + h - 80 - artY;

  { // full art
    const Texture2D *tex = imgCache_.Get(*card);
    Rectangle dst = {(float)artX, (float)artY, (float)artW, (float)artH};
    if (tex && tex->id != 0) {
      DrawTexturePro(*tex, {0, 0, (float)tex->width, (float)tex->height}, dst,
                     {0, 0}, 0, WHITE);
    } else {
      DrawRectangle(artX, artY, artW, artH, {col.r, col.g, col.b, 60});
      int nameW = MeasureText(card->name.c_str(), 14);
      DrawText(card->name.c_str(), artX + (artW - nameW) / 2,
               artY + artH / 2 - 7, 14, WHITE);
    }
    DrawRectangleLines(artX, artY, artW, artH, col);
    DrawText(card->cardTypeTag().c_str(), artX + 4, artY + 4, 12, col);
  }

  int infoY = artY + artH + 6;
  DrawText(card->statLine().c_str(), x + 8, infoY, 13, LIGHTGRAY);
  infoY += 18;

  int copies = countInDeck(card->cardNumber);
  Color cpCol = (copies >= kMaxCopies) ? RED : (copies > 0 ? GREEN : GRAY);
  DrawText(TextFormat("In deck: %d / %d", copies, kMaxCopies), x + 8, infoY, 12,
           cpCol);
  infoY += 16;

  const std::string &desc = card->description;
  int charsPerLine = (w - 16) / 7;
  int cur = 0;
  while (cur < (int)desc.size() && infoY < y + h - 4) {
    int end = std::min(cur + charsPerLine, (int)desc.size());
    if (end < (int)desc.size()) {
      int wb = (int)desc.rfind(' ', end);
      if (wb > cur)
        end = wb;
    }
    DrawText(desc.substr(cur, end - cur).c_str(), x + 8, infoY, 12,
             {200, 200, 200, 255});
    infoY += 15;
    cur = end;
    if (cur < (int)desc.size() && desc[cur] == ' ')
      ++cur;
  }
}

inline void DeckEditorScreen::drawDeckStats(int x, int y, int w) const {
  int mon = 0, spl = 0, trp = 0;
  for (const auto &c : deck_) {
    if (c.isMonster())
      ++mon;
    else if (c.isSpell())
      ++spl;
    else
      ++trp;
  }
  int total = (int)deck_.size();
  Color okCol = (total >= kMinDeckSize) ? GREEN : YELLOW;
  DrawText(TextFormat("%d/%d", total, kMinDeckSize), x, y, 15, okCol);
  drawProgressBar(x, y + 18, w, 8, (float)total / kMinDeckSize);
  y += 32;
  DrawText(("MON " + std::to_string(mon)).c_str(), x, y, 12,
           {180, 60, 60, 255});
  DrawText(("SPL " + std::to_string(spl)).c_str(), x + 55, y, 12,
           {0, 180, 140, 255});
  DrawText(("TRP " + std::to_string(trp)).c_str(), x + 110, y, 12,
           {160, 60, 200, 255});
}

inline void DeckEditorScreen::drawDeckPanel(int x, int y, int w, int h) const {
  Color border = !focusPool_ ? YELLOW : DARKGRAY;
  DrawRectangleLines(x, y, w, h, border);
  DrawText("Deck", x + 8, y + 6, 16, border);
  drawDeckStats(x + 8, y + 28, w - 16);

  const int itemH = 90;
  const int txW = 58;
  const int txH = itemH - 4;
  const int textX = x + 4 + txW + 6;
  int listY = y + 88;
  int maxVis = (y + h - listY - 4) / itemH;
  int scroll = std::max(0, deckCursor_ - maxVis / 2);

  for (int i = 0; i < maxVis && scroll + i < (int)deck_.size(); ++i) {
    int idx = scroll + i;
    const auto &c = deck_[idx];
    bool sel = !focusPool_ && idx == deckCursor_;
    Color col = sel ? YELLOW : cardTypeColor(c);
    int iy = listY + i * itemH;

    { // thumbnail
      Color tc = cardTypeColor(c);
      const Texture2D *tex = imgCache_.Get(c);
      Rectangle dst = {(float)(x + 4), (float)(iy + 2), (float)txW, (float)txH};
      if (tex && tex->id != 0)
        DrawTexturePro(*tex, {0, 0, (float)tex->width, (float)tex->height}, dst,
                       {0, 0}, 0, WHITE);
      else
        DrawRectangle(x + 4, iy + 2, txW, txH, Color{tc.r, tc.g, tc.b, 90});
      DrawRectangleLines(x + 4, iy + 2, txW, txH, col);
    }
    DrawText(c.cardTypeTag().c_str(), textX, iy + 6, 13, col);

    std::string stat = c.shortStat();
    if (!stat.empty())
      DrawText(stat.c_str(), textX, iy + 24, 12, Color{160, 160, 180, 255});

    std::string name = c.name;
    int maxNW = w - textX - 8;
    while (!name.empty() && MeasureText(name.c_str(), 14) > maxNW)
      name.pop_back();
    if (name.size() < c.name.size())
      name += "~";
    DrawText(name.c_str(), textX, iy + 44, 14, sel ? YELLOW : WHITE);

    if (sel)
      DrawRectangleLines(x + 2, iy, w - 4, itemH, YELLOW);
  }
}

// ── Draw
// ──────────────────────────────────────────────────────────────────────

inline void DeckEditorScreen::Draw() const {
  ClearBackground({15, 15, 20, 255});
  int sw = GetScreenWidth(), sh = GetScreenHeight();

  DrawRectangle(0, 0, sw, 28, {30, 30, 40, 255});
  DrawText("DECK EDITOR", 10, 6, 18, WHITE);
  DrawText("[ESC] clear status", sw - 160, 8, 13, GRAY);

  int padY = 32, padX = 6, padBot = 48;
  int panH = sh - padY - padBot;
  int poolW = sw * 38 / 100;
  int prevW = sw * 24 / 100;
  int deckW = sw - poolW - prevW - padX * 2;

  drawPoolPanel(padX, padY, poolW, panH);
  drawPreviewPanel(padX + poolW, padY, prevW, panH);
  drawDeckPanel(padX + poolW + prevW, padY, deckW, panH);

  int barY = sh - padBot + 4;
  DrawRectangle(0, sh - padBot, sw, padBot, {25, 25, 35, 255});
  DrawText("[TAB] switch  [Arrow Keys] navigate  [PgUp/Dn] fast scroll  "
           "[ENTER/A] add  [DEL/D] remove  [L] search  [O] sort  [T] filter  "
           "[C] clear  [S] save  [L] load  [F] duel (40+)",
           8, barY, 11, LIGHTGRAY);
  if (!statusMsg_.empty())
    DrawText(statusMsg_.c_str(), 8, barY + 18, 14, GREEN);
}

// ── Persistence
// ───────────────────────────────────────────────────────────────

inline bool DeckEditorScreen::SaveDeck(const std::string &name) const {
  std::filesystem::path dir = std::filesystem::current_path();
  dir /= "data";
  dir /= "decks";
  std::filesystem::create_directories(dir);
  std::ofstream f(dir / (name + ".txt"));
  if (!f.is_open())
    return false;
  for (const auto &c : deck_)
    f << c.cardNumber << "\n";
  return true;
}

inline bool DeckEditorScreen::LoadDeck(const std::string &name) {
  std::filesystem::path path =
      std::filesystem::current_path() / "data" / "decks" / (name + ".txt");
  std::ifstream f(path);
  if (!f.is_open())
    return false;
  deck_.clear();
  std::string line;
  while (std::getline(f, line)) {
    if (line.empty() || line[0] == '#')
      continue;
    try {
      uint32_t id = (uint32_t)std::stoul(line);
      const auto *card = db_.GetCardById(id);
      if (card)
        deck_.push_back(*card);
    } catch (...) {
    }
  }
  deckCursor_ = 0;
  return !deck_.empty();
}

} // namespace openjoey::ui
