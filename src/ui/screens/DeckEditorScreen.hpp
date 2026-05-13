#pragma once
#include "card/Card.hpp"
#include "ui/core/AppContext.hpp"
#include "ui/core/AppScreen.hpp"
#include "ui/core/Theme.hpp"
#include "ui/screens/IScreen.hpp"
#include "ui/widgets/display/CardGrid.hpp"
#include "ui/widgets/display/CardList.hpp"
#include "ui/widgets/display/CardPreview.hpp"
#include "ui/widgets/display/DeckStats.hpp"
#include "ui/widgets/input/KeyboardNav.hpp"
#include "ui/widgets/input/TextInput.hpp"
#include "ui/widgets/layout/Panel.hpp"
#include "ui/widgets/layout/ScreenChrome.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <raylib.h>
#include <string>
#include <utility>
#include <vector>

namespace openjoey::ui {

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
  Id,
  COUNT
};
enum class DeckTypeFilter { All, Monster, Spell, Trap, COUNT };

class DeckEditorScreen : public IScreen {
public:
  static constexpr int kMinDeckSize = 40;
  static constexpr int kMaxDeckSize = 60;
  static constexpr int kMaxCopies = 3;

  explicit DeckEditorScreen(AppContext &ctx) : ctx_(ctx) {
    loadBackground(ContentPaths::deckEditorBgImg());
    rebuildPool();
  }

  ~DeckEditorScreen() { unloadBackground(); }

  ScreenEvent Update(float /*dt*/) override;
  void Draw() const override;

  bool SaveDeck(const std::string &name) const;
  bool LoadDeck(const std::string &name);

private:
  AppContext &ctx_;

  std::vector<openjoey::Card> pool_;
  std::vector<openjoey::Card> deck_;

  TextInput searchInput_;
  bool focusPool_ = true;
  bool deckGridView_ = false;
  KeyboardNav poolNav_;
  KeyboardNav deckNav_;

  DeckSortMode sortMode_ = DeckSortMode::Type;
  DeckTypeFilter typeFilter_ = DeckTypeFilter::All;

  std::string statusMsg_;
  mutable CardPreview preview_;

  void rebuildPool();
  std::vector<const openjoey::Card *> filteredPool() const;
  int countInDeck(uint32_t id) const;

  void handlePoolInput(const std::vector<const openjoey::Card *> &fp);
  void handleDeckInput();
  ScreenEvent handleGlobalInput(const std::vector<const openjoey::Card *> &fp);

  static const char *sortModeLabel(DeckSortMode m);
  static const char *typeFilterLabel(DeckTypeFilter f);
};

// ── Pool rebuild

inline void DeckEditorScreen::rebuildPool() {
  using CmpFn = bool (*)(const openjoey::Card &, const openjoey::Card &);
  pool_.clear();
  for (const auto &c : ctx_.cardRepo->all())
    pool_.push_back(c);

  static constexpr std::pair<CmpFn, bool> kSort[] = {
      {openjoey::Card::sortByCardType, false},
      {openjoey::Card::sortByName, false},
      {openjoey::Card::sortByName, true},
      {openjoey::Card::sortByLevel, false},
      {openjoey::Card::sortByLevel, true},
      {openjoey::Card::sortByAtk, false},
      {openjoey::Card::sortByAtk, true},
      {openjoey::Card::sortByDef, false},
      {openjoey::Card::sortByDef, true},
      {openjoey::Card::sortById, false},
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

inline ScreenEvent DeckEditorScreen::Update(float /*dt*/) {
  ctx_.imageCache.PollAndLoad();
  if (IsKeyPressed(KEY_ESCAPE) && !searchInput_.isTyping())
    return ScreenEvent::replace(AppScreen::MainMenu);
  searchInput_.Update();
  if (searchInput_.isChanged())
    poolNav_.cursor = 0;
  auto fp = filteredPool();
  if (focusPool_)
    handlePoolInput(fp);
  else
    handleDeckInput();
  return handleGlobalInput(fp);
}

inline void DeckEditorScreen::handlePoolInput(
    const std::vector<const openjoey::Card *> &fp) {
  poolNav_.setCount((int)fp.size());
  poolNav_.handleClampKeys();

  if (IsKeyPressed(KEY_ENTER) && !fp.empty()) {
    const auto &card = *fp[poolNav_.cursor];
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
}

inline void DeckEditorScreen::handleDeckInput() {
  const int deckSz = (int)deck_.size();
  const int step = deckGridView_ ? CardGrid::ColCount() : 1;
  deckNav_.setCount(deckSz);

  if (IsKeyPressed(KEY_DOWN))
    deckNav_.clampNext(step);
  if (IsKeyPressed(KEY_UP))
    deckNav_.clampPrev(step);
  if (IsKeyPressed(KEY_PAGE_DOWN))
    deckNav_.clampNext(step * 3);
  if (IsKeyPressed(KEY_PAGE_UP))
    deckNav_.clampPrev(step * 3);
  if (deckGridView_) {
    if (IsKeyPressed(KEY_RIGHT))
      deckNav_.clampNext();
    if (IsKeyPressed(KEY_LEFT))
      deckNav_.clampPrev();
  }

  if ((IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE) ||
       IsKeyPressed(KEY_D)) &&
      deckSz > 0) {
    statusMsg_ = "Removed: " + deck_[deckNav_.cursor].name;
    deck_.erase(deck_.begin() + deckNav_.cursor);
    deckNav_.setCount((int)deck_.size());
  }
  if (IsKeyPressed(KEY_G)) {
    deckGridView_ = !deckGridView_;
    statusMsg_ = deckGridView_ ? "Deck: grid view" : "Deck: list view";
  }
  if (IsKeyPressed(KEY_TAB) || (!deckGridView_ && IsKeyPressed(KEY_LEFT)))
    focusPool_ = true;
}

inline ScreenEvent DeckEditorScreen::handleGlobalInput(
    const std::vector<const openjoey::Card *> & /*fp*/) {
  if (IsKeyPressed(KEY_O)) {
    sortMode_ =
        static_cast<DeckSortMode>((static_cast<int>(sortMode_) + 1) %
                                  static_cast<int>(DeckSortMode::COUNT));
    rebuildPool();
    poolNav_.cursor = 0;
    statusMsg_ = std::string("Sort: ") + sortModeLabel(sortMode_);
  }
  if (IsKeyPressed(KEY_T)) {
    typeFilter_ =
        static_cast<DeckTypeFilter>((static_cast<int>(typeFilter_) + 1) %
                                    static_cast<int>(DeckTypeFilter::COUNT));
    poolNav_.cursor = 0;
    statusMsg_ = std::string("Filter: ") + typeFilterLabel(typeFilter_);
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
    deckNav_.cursor = 0;
    statusMsg_ = "Deck cleared";
  }
  if (IsKeyPressed(KEY_F)) {
    if ((int)deck_.size() >= kMinDeckSize) {
      ctx_.selectedDeck = deck_;
      return ScreenEvent::replace(AppScreen::Duel);
    }
    statusMsg_ = "Need " + std::to_string(kMinDeckSize - (int)deck_.size()) +
                 " more cards";
  }
  if (IsKeyPressed(KEY_ESCAPE))
    statusMsg_.clear();
  return ScreenEvent::none();
}

// ── Filtered pool

inline std::vector<const openjoey::Card *>
DeckEditorScreen::filteredPool() const {
  std::vector<const openjoey::Card *> out;
  std::string q = searchInput_.GetText();
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

// ── Draw

inline void DeckEditorScreen::Draw() const {
  const Theme t = Theme::FromScreen();

  ClearBackground(t.colors.bgMain);
  if (background_.id) {
    const float scaleX = (float)t.sw / background_.width;
    const float scaleY = (float)t.sh / background_.height;
    DrawTextureEx(background_, {0.f, 0.f}, 0.f, fmaxf(scaleX, scaleY), WHITE);
  }

  ScreenChrome::DrawHeader(0, 0, t.sw, t.headerHeight, "DECK EDITOR",
                           "[ESC] clear status", WHITE, t);

  const int panH = t.sh - t.mainPadY - t.mainPadBottom;
  const int poolW = t.sw * t.poolWidthPct / 100;
  const int prevW = t.sw * t.previewWidthPct / 100;
  const int deckW = t.sw - poolW - prevW - t.mainPadX * 2;
  const int prevX = t.mainPadX;    // poolX + poolW;
  const int poolX = prevX + prevW; // t.mainPadX;
  const int deckX = poolW + prevW;

  auto fp = filteredPool();

  // Pool panel
  {
    std::string badge = std::string(sortModeLabel(sortMode_)) + "  [" +
                        typeFilterLabel(typeFilter_) + "]  " +
                        std::to_string(fp.size()) + " cards";
    Panel::Draw("Card Pool", badge.c_str(), poolX, t.mainPadY, poolW, panH,
                focusPool_);
    int searchY = t.mainPadY + t.searchBarYOffset;
    searchInput_.Draw(poolX + t.thumbnailPad, searchY,
                      poolW - t.thumbnailPad * 2, t.searchBarHeight(panH));
    int listY = searchY + t.listYOffset;
    CardList::Draw(fp, ctx_.imageCache, poolX, listY, poolW,
                   t.mainPadY + panH - listY, poolNav_.cursor, focusPool_,
                   kMaxCopies, [this](uint32_t id) { return countInDeck(id); });
  }

  // Preview panel
  {
    const openjoey::Card *card = nullptr;
    if (focusPool_ && !fp.empty() && poolNav_.cursor < (int)fp.size())
      card = fp[poolNav_.cursor];
    else if (!focusPool_ && !deck_.empty() &&
             deckNav_.cursor < (int)deck_.size())
      card = &deck_[deckNav_.cursor];
    preview_.SetCard(card);
    preview_.Draw({(float)prevX, (float)t.mainPadY, (float)prevW, (float)panH},
                  ctx_.imageCache);
  }

  // Deck panel
  {
    Color border = !focusPool_ ? YELLOW : DARKGRAY;
    DrawRectangleLines(deckX, t.mainPadY, deckW, panH, border);
    std::string title = deckGridView_ ? "Deck [Grid]" : "Deck [List]";
    DrawText(title.c_str(), deckX + t.previewPadX, t.mainPadY + t.cardTypeY,
             t.fontPanelTitle, border);
    DeckStats::Draw(deck_, kMinDeckSize, deckX + t.previewPadX,
                    t.mainPadY + t.searchBarYOffset,
                    deckW - t.previewArtSidePad);
    std::vector<const openjoey::Card *> deckPtrs;
    deckPtrs.reserve(deck_.size());
    for (const auto &c : deck_)
      deckPtrs.push_back(&c);
    int listY = t.mainPadY + t.deckListYOffset;
    if (deckGridView_)
      CardGrid::Draw(deckPtrs, ctx_.imageCache, deckX, listY, deckW,
                     t.mainPadY + panH - listY, deckNav_.cursor, !focusPool_);
    else
      CardList::Draw(deckPtrs, ctx_.imageCache, deckX, listY, deckW,
                     t.mainPadY + panH - listY, deckNav_.cursor, !focusPool_,
                     kMaxCopies,
                     [this](uint32_t id) { return countInDeck(id); });
  }

  ScreenChrome::DrawFooter(
      0, t.sh - t.mainPadBottom, t.sw, t.mainPadBottom,
      "[TAB] switch  [Arrows] navigate  [PgUp/Dn] fast scroll  "
      "[ENTER] add  [DEL/D] remove  [O] sort  [T] filter  "
      "[G] grid/list  [C] clear  [S] save  [L] load  [F] duel (40+)",
      t);
  if (!statusMsg_.empty()) {
    const int barY = t.sh - t.mainPadBottom + t.statusBarYOffset;
    DrawText(statusMsg_.c_str(), t.previewPadX, barY + t.fontHelpText + 2,
             t.fontCardName, GREEN);
  }
}

// ── Persistence

inline bool DeckEditorScreen::SaveDeck(const std::string &name) const {
  std::filesystem::path dir =
      std::filesystem::current_path() / "data" / "decks";
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
      const auto *card = ctx_.cardRepo->getById(id);
      if (card && (int)deck_.size() < kMaxDeckSize)
        deck_.push_back(*card);
    } catch (...) {
    }
  }
  deckNav_.cursor = 0;
  return !deck_.empty();
}

} // namespace openjoey::ui
