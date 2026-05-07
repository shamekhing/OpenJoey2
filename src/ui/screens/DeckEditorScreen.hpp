#pragma once
#include "card/Card.hpp"
#include "card/CardDatabase.hpp"
#include "ui/AppScreen.hpp"
#include "ui/CardImageCache.hpp"
#include "ui/StyleSheet.hpp"
#include "ui/screens/widgets/DeckStats.hpp"
#include "ui/screens/widgets/Grid.hpp"
#include "ui/screens/widgets/Header.hpp"
#include "ui/screens/widgets/List.hpp"

#include "ui/screens/widgets/TextInput.hpp"
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
  Type, NameDesc, NameAsc, LevelDesc, LevelAsc,
  AtkDesc, AtkAsc, DefDesc, DefAsc, Id, COUNT
};
enum class DeckTypeFilter { All, Monster, Spell, Trap, COUNT };

class DeckEditorScreen {
public:
  static constexpr int kMinDeckSize = 40;
  static constexpr int kMaxDeckSize = 60;
  static constexpr int kMaxCopies   = 3;

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

  openjoey::ui::TextInput searchInput_;
  bool focusPool_    = true;
  bool deckGridView_ = false;   // toggle between list and grid for the deck panel
  int  poolCursor_   = 0;
  int  deckCursor_   = 0;

  DeckSortMode   sortMode_   = DeckSortMode::Type;
  DeckTypeFilter typeFilter_ = DeckTypeFilter::All;

  AppScreen next_      = AppScreen::DeckEditor;
  bool      deckReady_ = false;
  std::string statusMsg_;

  mutable CardImageCache imgCache_;

  void rebuildPool();
  std::vector<const openjoey::Card *> filteredPool() const;
  int countInDeck(uint32_t id) const;

  void drawPoolPanel(int x, int y, int w, int h) const;
  void drawPreviewPanel(int x, int y, int w, int h) const;
  void drawDeckPanel(int x, int y, int w, int h) const;

  static const char *sortModeLabel(DeckSortMode m);
  static const char *typeFilterLabel(DeckTypeFilter f);
};

// ── Pool rebuild

inline void DeckEditorScreen::rebuildPool() {
  pool_.clear();
  for (const auto &c : db_.GetAllCards())
    pool_.push_back(c);

  static constexpr std::pair<cmpFunArrPointer, bool> kSort[] = {
      {openjoey::Card::sortByCardType, false},
      {openjoey::Card::sortByName,     false},
      {openjoey::Card::sortByName,     true},
      {openjoey::Card::sortByLevel,    false},
      {openjoey::Card::sortByLevel,    true},
      {openjoey::Card::sortByAtk,      false},
      {openjoey::Card::sortByAtk,      true},
      {openjoey::Card::sortByDef,      false},
      {openjoey::Card::sortByDef,      true},
      {openjoey::Card::sortById,       false},
  };
  auto [cmp, rev] = kSort[(int)sortMode_];
  std::sort(pool_.begin(), pool_.end(), cmp);
  if (rev)
    std::reverse(pool_.begin(), pool_.end());
}

// ── Static label methods

inline const char *DeckEditorScreen::sortModeLabel(DeckSortMode m) {
  static constexpr const char *kLabels[] = {
      "Type",       "Name (A-Z)", "Name (Z-A)", "Level (desc)", "Level (asc)",
      "ATK (desc)", "ATK (asc)",  "DEF (desc)", "DEF (asc)",    "ID"};
  int idx = (int)m;
  return (idx >= 0 && idx < (int)DeckSortMode::COUNT) ? kLabels[idx] : "?";
}

inline const char *DeckEditorScreen::typeFilterLabel(DeckTypeFilter f) {
  static constexpr const char *kLabels[] = {"All", "Monster", "Spell", "Trap"};
  int idx = (int)f;
  return (idx >= 0 && idx < (int)DeckTypeFilter::COUNT) ? kLabels[idx] : "?";
}

// ── Update

inline void DeckEditorScreen::Update() {
  imgCache_.PollAndLoad();

  if (IsKeyPressed(KEY_ESCAPE) && !searchInput_.isTyping()) {
    next_ = AppScreen::MainMenu;
  }

  {
    searchInput_.Update();
    poolCursor_ = searchInput_.isChanged() ? 0 : poolCursor_;
  }

  if (IsKeyPressed(KEY_O)) {
    sortMode_ = static_cast<DeckSortMode>(
        (static_cast<int>(sortMode_) + 1) % static_cast<int>(DeckSortMode::COUNT));
    rebuildPool();
    poolCursor_ = 0;
    statusMsg_  = std::string("Sort: ") + sortModeLabel(sortMode_);
  }
  if (IsKeyPressed(KEY_T)) {
    typeFilter_ = static_cast<DeckTypeFilter>(
        (static_cast<int>(typeFilter_) + 1) % static_cast<int>(DeckTypeFilter::COUNT));
    poolCursor_ = 0;
    statusMsg_  = std::string("Filter: ") + typeFilterLabel(typeFilter_);
  }

  auto fp    = filteredPool();
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

    if (IsKeyPressed(KEY_ENTER) && poolSz > 0) {
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

    if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_RIGHT))
      focusPool_ = false;

  } else {
    // Deck navigation — step size differs between list and grid views
    const int step = deckGridView_ ? Grid::ColCount() : 1;

    if (IsKeyPressed(KEY_DOWN))
      deckCursor_ = std::min(deckCursor_ + step, deckSz - 1);
    if (IsKeyPressed(KEY_UP))
      deckCursor_ = std::max(deckCursor_ - step, 0);
    if (IsKeyPressed(KEY_PAGE_DOWN))
      deckCursor_ = std::min(deckCursor_ + step * 3, deckSz - 1);
    if (IsKeyPressed(KEY_PAGE_UP))
      deckCursor_ = std::max(deckCursor_ - step * 3, 0);

    // In grid mode LEFT/RIGHT move within the row; in list mode LEFT exits to pool
    if (deckGridView_) {
      if (IsKeyPressed(KEY_RIGHT))
        deckCursor_ = std::min(deckCursor_ + 1, deckSz - 1);
      if (IsKeyPressed(KEY_LEFT))
        deckCursor_ = std::max(deckCursor_ - 1, 0);
    }

    if ((IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE) ||
         IsKeyPressed(KEY_D)) && deckSz > 0) {
      statusMsg_ = "Removed: " + deck_[deckCursor_].name;
      deck_.erase(deck_.begin() + deckCursor_);
      if (deckCursor_ >= (int)deck_.size() && deckCursor_ > 0)
        --deckCursor_;
    }

    if (IsKeyPressed(KEY_G)) {
      deckGridView_ = !deckGridView_;
      statusMsg_ = deckGridView_ ? "Deck: grid view" : "Deck: list view";
    }

    if (IsKeyPressed(KEY_TAB) || (!deckGridView_ && IsKeyPressed(KEY_LEFT)))
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
    statusMsg_  = "Deck cleared";
  }
  if (IsKeyPressed(KEY_F)) {
    if ((int)deck_.size() >= kMinDeckSize) {
      deckReady_ = true;
      next_      = AppScreen::Duel;
    } else {
      statusMsg_ = "Need " +
                   std::to_string(kMinDeckSize - (int)deck_.size()) +
                   " more cards";
    }
  }
  if (IsKeyPressed(KEY_ESCAPE))
    statusMsg_.clear();
}

// ── Filtered pool

inline std::vector<const openjoey::Card *>
DeckEditorScreen::filteredPool() const {
  std::vector<const openjoey::Card *> out;
  std::string q = searchInput_.GetText();
  std::transform(q.begin(), q.end(), q.begin(), ::tolower);

  for (const auto &c : pool_) {
    if (typeFilter_ == DeckTypeFilter::Monster && !c.isMonster()) continue;
    if (typeFilter_ == DeckTypeFilter::Spell   && !c.isSpell())   continue;
    if (typeFilter_ == DeckTypeFilter::Trap    && !c.isTrap())    continue;
    if (!q.empty()) {
      std::string lo = c.name;
      std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower);
      if (lo.find(q) == std::string::npos) continue;
    }
    out.push_back(&c);
  }
  return out;
}

inline int DeckEditorScreen::countInDeck(uint32_t id) const {
  int n = 0;
  for (const auto &c : deck_)
    if (c.cardNumber == id) ++n;
  return n;
}

// ── Draw helpers

inline void DeckEditorScreen::drawPoolPanel(int x, int y, int w, int h) const {
  auto fp = filteredPool();
  std::string badge = std::string(sortModeLabel(sortMode_)) + "  [" +
                      typeFilterLabel(typeFilter_) + "]  " +
                      std::to_string(fp.size()) + " cards";

  Header::Draw("Card Pool", badge, x, y, w, h, focusPool_);

  int searchY = y + SEARCH_BAR_Y_OFFSET;
  searchInput_.Draw(x + THUMBNAIL_PAD, searchY,
                    w - THUMBNAIL_PAD * 2, SEARCH_BAR_HEIGHT(h));

  int listY = searchY + LIST_Y_OFFSET;
  List::Draw(fp, imgCache_, x, listY, w, y + h - listY,
             poolCursor_, focusPool_, kMaxCopies,
             [this](uint32_t id) { return countInDeck(id); });
}

inline void DeckEditorScreen::drawPreviewPanel(int x, int y, int w,
                                               int h) const {
  DrawRectangleLines(x, y, w, h, DARKGRAY);
  DrawText("Preview", x + PREVIEW_PAD_X, y + CARD_TYPE_Y,
           FONT_CARD_NAME, DARKGRAY);

  auto fp = filteredPool();
  const openjoey::Card *card = nullptr;
  if (focusPool_ && !fp.empty() && poolCursor_ < (int)fp.size())
    card = fp[poolCursor_];
  else if (!focusPool_ && !deck_.empty() && deckCursor_ < (int)deck_.size())
    card = &deck_[deckCursor_];

  if (!card)
    return;

  Color col  = ListItem::cardTypeColor(*card);
  int artX   = x + PREVIEW_PAD_X;
  int artY   = y + PREVIEW_ART_Y_OFFSET;
  int artW   = w - PREVIEW_ART_SIDE_PAD;
  int artH   = (int)((float)artW * PREVIEW_ASPECT_RATIO);
  if (artY + artH > y + h - MAIN_PAD_BOTTOM / 2)
    artH = y + h - MAIN_PAD_BOTTOM / 2 - artY;

  {
    const Texture2D *tex = imgCache_.Get(*card);
    Rectangle dst = {(float)artX, (float)artY, (float)artW, (float)artH};
    if (tex && tex->id != 0) {
      DrawTexturePro(*tex, {0, 0, (float)tex->width, (float)tex->height},
                     dst, {0, 0}, 0, WHITE);
    } else {
      DrawRectangle(artX, artY, artW, artH,
                    Color{col.r, col.g, col.b, CARD_PREVIEW_ALPHA});
      int nameW = MeasureText(card->name.c_str(), FONT_CARD_NAME);
      DrawText(card->name.c_str(), artX + (artW - nameW) / 2,
               artY + artH / 2 - FONT_CARD_NAME / 2, FONT_CARD_NAME, WHITE);
    }
    DrawRectangleLines(artX, artY, artW, artH, col);
    DrawText(card->cardTypeTag().c_str(), artX + THUMBNAIL_PAD,
             artY + THUMBNAIL_PAD, FONT_CARD_STAT, col);
  }

  int infoY = artY + artH + PREVIEW_INFO_GAP;
  DrawText(card->statLine().c_str(), x + PREVIEW_PAD_X, infoY,
           FONT_CARD_TYPE, LIGHTGRAY);
  infoY += FONT_CARD_TYPE + PREVIEW_INFO_GAP;

  int copies = countInDeck(card->cardNumber);
  Color cpCol = (copies >= kMaxCopies) ? RED : (copies > 0 ? GREEN : GRAY);
  DrawText(TextFormat("In deck: %d / %d", copies, kMaxCopies),
           x + PREVIEW_PAD_X, infoY, FONT_CARD_STAT, cpCol);
  infoY += FONT_CARD_STAT + PREVIEW_INFO_GAP;

  const std::string &desc = card->description;
  int charsPerLine = (w - PREVIEW_DESC_SIDE_PAD) / std::max(1, PREVIEW_DESC_CHAR_W);
  int cur = 0;
  int lineH = FONT_CARD_STAT + 3;
  while (cur < (int)desc.size() && infoY < y + h - THUMBNAIL_PAD) {
    int end = std::min(cur + charsPerLine, (int)desc.size());
    if (end < (int)desc.size()) {
      int wb = (int)desc.rfind(' ', end);
      if (wb > cur) end = wb;
    }
    DrawText(desc.substr(cur, end - cur).c_str(), x + PREVIEW_PAD_X, infoY,
             FONT_CARD_STAT, COLOR_DESC_TEXT);
    infoY += lineH;
    cur = end;
    if (cur < (int)desc.size() && desc[cur] == ' ') ++cur;
  }
}

inline void DeckEditorScreen::drawDeckPanel(int x, int y, int w, int h) const {
  Color border = !focusPool_ ? YELLOW : DARKGRAY;
  DrawRectangleLines(x, y, w, h, border);

  std::string title = deckGridView_ ? "Deck [Grid]" : "Deck [List]";
  DrawText(title.c_str(), x + PREVIEW_PAD_X, y + CARD_TYPE_Y,
           FONT_PANEL_TITLE, border);

  DeckStats::Draw(deck_, kMinDeckSize,
                  x + PREVIEW_PAD_X, y + SEARCH_BAR_Y_OFFSET,
                  w - PREVIEW_ART_SIDE_PAD);

  std::vector<const openjoey::Card *> deckPtrs;
  deckPtrs.reserve(deck_.size());
  for (const auto &c : deck_)
    deckPtrs.push_back(&c);

  int listY = y + DECK_LIST_Y_OFFSET;
  int listH = y + h - listY;

  if (deckGridView_) {
    Grid::Draw(deckPtrs, imgCache_, x, listY, w, listH,
               deckCursor_, !focusPool_);
  } else {
    List::Draw(deckPtrs, imgCache_, x, listY, w, listH,
               deckCursor_, !focusPool_, kMaxCopies,
               [this](uint32_t id) { return countInDeck(id); });
  }
}

// ── Draw

inline void DeckEditorScreen::Draw() const {
  ClearBackground(COLOR_BG_MAIN);
  const int sw = GetScreenWidth();
  const int sh = GetScreenHeight();

  DrawRectangle(0, 0, sw, HEADER_HEIGHT, COLOR_HEADER_BG);
  DrawText("DECK EDITOR", HEADER_TITLE_X, HEADER_TITLE_Y,
           FONT_SCREEN_TITLE, WHITE);
  DrawText("[ESC] clear status", sw - HELP_TEXT_X_OFFSET, HELP_TEXT_Y,
           FONT_CARD_TYPE, GRAY);

  const int padY   = MAIN_PAD_Y;
  const int padX   = MAIN_PAD_X;
  const int padBot = MAIN_PAD_BOTTOM;
  const int panH   = sh - padY - padBot;
  const int poolW  = sw * POOL_WIDTH_PERCENT / 100;
  const int prevW  = sw * PREVIEW_WIDTH_PERCENT / 100;
  const int deckW  = sw - poolW - prevW - padX * 2;

  drawPoolPanel(padX, padY, poolW, panH);
  drawPreviewPanel(padX + poolW, padY, prevW, panH);
  drawDeckPanel(padX + poolW + prevW, padY, deckW, panH);

  const int barY = sh - padBot + STATUS_BAR_Y_OFFSET;
  DrawRectangle(0, sh - padBot, sw, padBot, COLOR_FOOTER_BG);
  DrawText("[TAB] switch  [Arrows] navigate  [PgUp/Dn] fast scroll  "
           "[ENTER] add  [DEL/D] remove  [O] sort  [T] filter  "
           "[G] grid/list  [C] clear  [S] save  [L] load  [F] duel (40+)",
           PREVIEW_PAD_X, barY, FONT_HELP_TEXT, LIGHTGRAY);
  if (!statusMsg_.empty())
    DrawText(statusMsg_.c_str(), PREVIEW_PAD_X, barY + FONT_HELP_TEXT + 2,
             FONT_CARD_NAME, GREEN);
}

// ── Persistence

inline bool DeckEditorScreen::SaveDeck(const std::string &name) const {
  std::filesystem::path dir =
      std::filesystem::current_path() / "data" / "decks";
  std::filesystem::create_directories(dir);
  std::ofstream f(dir / (name + ".txt"));
  if (!f.is_open()) return false;
  for (const auto &c : deck_)
    f << c.cardNumber << "\n";
  return true;
}

inline bool DeckEditorScreen::LoadDeck(const std::string &name) {
  std::filesystem::path path =
      std::filesystem::current_path() / "data" / "decks" / (name + ".txt");
  std::ifstream f(path);
  if (!f.is_open()) return false;
  deck_.clear();
  std::string line;
  while (std::getline(f, line)) {
    if (line.empty() || line[0] == '#') continue;
    try {
      uint32_t id      = (uint32_t)std::stoul(line);
      const auto *card = db_.GetCardById(id);
      if (card) deck_.push_back(*card);
    } catch (...) {}
  }
  deckCursor_ = 0;
  return !deck_.empty();
}

} // namespace openjoey::ui
