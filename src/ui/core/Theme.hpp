#pragma once
#include <raylib.h>

// Theme replaces the raw macros in StyleSheet.hpp with a computed, typed
// struct. Call Theme::FromScreen() once at the top of each Draw() and cache
// the result — every field is a plain int or float, no hidden raylib calls.
namespace openjoey::ui {

struct Theme {
    // ── Screen size (snapshot at construction time)
    int sw, sh;

    // ── Font sizes
    int fontMainTitle;    // ~48px at 1280×720
    int fontMenuItem;     // ~28px
    int fontScreenTitle;  // ~18px
    int fontPanelTitle;   // ~16px
    int fontDeckStats;    // ~15px
    int fontCardName;     // ~14px
    int fontCardType;     // ~13px
    int fontCardStat;     // ~12px
    int fontHelpText;     // ~11px
    int fontHelpSmall;    // ~16px

    // ── Screen chrome
    int headerHeight;
    int headerTitleX;
    int headerTitleY;
    int helpTextXOffset;
    int helpTextY;
    int mainPadX;
    int mainPadY;
    int mainPadBottom;
    int statusBarYOffset;

    // ── Panel layout percentages (multiply by sw)
    int poolWidthPct    = 38;
    int previewWidthPct = 24;

    // ── Card list / thumbnail
    int cardItemHeight;
    int thumbnailWidth;
    int thumbnailPad;
    int thumbnailTextGap;
    int searchBarYOffset;
    int listYOffset;
    int cardTypeY;
    int cardStatY;
    int cardNameY;
    int cardNameRightMargin;
    int copyCountXOffset;
    int selectionBorder;
    int deckListYOffset;

    // ── Scrollbar
    int scrollbarWidth;
    int scrollbarXOffset;
    int scrollbarHTrim;
    int scrollbarThumbMin;

    // ── Preview panel
    int   previewPadX;
    int   previewArtYOffset;
    int   previewArtSidePad;
    float previewAspectRatio = 1.45f;
    int   previewInfoGap;
    int   previewDescSidePad;

    // ── Deck stats label x-offsets
    int statSplXOffset;
    int statTrpXOffset;

    // ── Main menu
    int menuArrowOffset;
    int menuItemSpacing;
    int menuHelpBottomOffset;

    // ── Grid view
    int gridCols = 4;
    int gridGap;
    int gridLabelH;

    // ── Duel screen layout percentage (multiply by sw)
    int duelLeftWPct  = 25;
    int duelRightWPct = 17;

    // ── Colors (palette constants — same regardless of resolution)
    struct Palette {
        Color bgDark         = {10,  10,  20,  255};
        Color bgMain         = {15,  15,  20,  255};
        Color headerBg       = {30,  30,  40,  255};
        Color footerBg       = {25,  25,  35,  255};
        Color statText       = {160, 160, 180, 255};
        Color descText       = {200, 200, 200, 255};
        Color progressBg     = {40,  40,  40,  255};
        Color scrollbarBg    = {40,  40,  60,  255};
        Color scrollbarThumb = {160, 160, 200, 200};
        Color monsterStat    = {180, 60,  60,  255};
        Color spellStat      = {0,   180, 140, 255};
        Color trapStat       = {160, 60,  200, 255};
        Color fieldMat       = {14,  20,  14,  255};
        Color dividerLine    = {50,  50,  80,  255};
        Color dividerMid     = {80,  80,  110, 220};
        Color panelBg        = {16,  16,  26,  255};
        Color panelBorder    = {55,  55,  85,  255};
        Color cardBackFg     = {8,   6,   42,  255};
    } colors;

    // Build a Theme from the current window size. Call once per frame.
    static Theme FromScreen() {
        Theme t;
        t.sw = GetScreenWidth();
        t.sh = GetScreenHeight();
        int sw = t.sw, sh = t.sh;

        t.fontMainTitle    = int(0.067f * sh);
        t.fontMenuItem     = int(0.039f * sh);
        t.fontScreenTitle  = int(0.025f * sh);
        t.fontPanelTitle   = int(0.023f * sh);
        t.fontDeckStats    = int(0.021f * sh);
        t.fontCardName     = int(0.020f * sh);
        t.fontCardType     = int(0.0181f * sh);
        t.fontCardStat     = int(0.017f * sh);
        t.fontHelpText     = int(0.0153f * sh);
        t.fontHelpSmall    = int(0.023f * sh);

        t.headerHeight     = int(0.039f * sh);
        t.headerTitleX     = int(0.008f * sw);
        t.headerTitleY     = int(0.009f * sh);
        t.helpTextXOffset  = int(0.125f * sw);
        t.helpTextY        = int(0.012f * sh);
        t.mainPadX         = int(0.0047f * sw);
        t.mainPadY         = int(0.045f * sh);
        t.mainPadBottom    = int(0.067f * sh);
        t.statusBarYOffset = int(0.006f * sh);

        t.cardItemHeight      = int(0.125f * sh);
        t.thumbnailWidth      = int(0.081f * sh);
        t.thumbnailPad        = int(0.006f * sh);
        t.thumbnailTextGap    = int(0.009f * sh);
        t.searchBarYOffset    = int(0.039f * sh);
        t.listYOffset         = int(0.050f * sh);
        t.cardTypeY           = int(0.009f * sh);
        t.cardStatY           = int(0.034f * sh);
        t.cardNameY           = int(0.062f * sh);
        t.cardNameRightMargin = int(0.027f * sw);
        t.copyCountXOffset    = int(0.027f * sw);
        t.selectionBorder     = int(0.003f * sh + 0.5f);
        t.deckListYOffset     = int(0.123f * sh);

        t.scrollbarWidth    = int(0.0025f * sw);
        t.scrollbarXOffset  = int(0.004f * sw);
        t.scrollbarHTrim    = int(0.084f * sh);
        t.scrollbarThumbMin = int(0.012f * sh);

        t.previewPadX      = int(0.006f * sw);
        t.previewArtYOffset = int(0.039f * sh);
        t.previewArtSidePad = int(0.013f * sw);
        t.previewInfoGap    = int(0.009f * sh);
        t.previewDescSidePad = int(0.013f * sw);

        t.statSplXOffset = int(0.043f * sw);
        t.statTrpXOffset = int(0.086f * sw);

        t.menuArrowOffset       = int(0.019f * sw);
        t.menuItemSpacing       = int(0.056f * sh);
        t.menuHelpBottomOffset  = int(0.042f * sh);

        t.gridGap    = int(0.0047f * sw);
        t.gridLabelH = int(0.023f * sh);

        return t;
    }

    // Panel-relative helpers (pass panel height h)
    int panelHeaderFont(int h) const { return int(0.03f * h); }
    int searchBarHeight(int h) const { return int(0.05f * h); }

    // Grid helpers (pass panel width)
    int gridCardW(int panelW) const {
        return (panelW - gridGap * (gridCols + 1)) / gridCols;
    }
    int gridCardH(int panelW) const {
        return int((float)gridCardW(panelW) * previewAspectRatio);
    }
    int gridCellH(int panelW) const {
        return gridCardH(panelW) + gridLabelH + gridGap;
    }
};

} // namespace openjoey::ui
