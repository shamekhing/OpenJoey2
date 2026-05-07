#pragma once
#include "ui/platform/AppConfig.hpp"
#include <raylib.h>

// ── Responsive design: all sizes derived from live screen dimensions.
// _SH / _SW expand to raylib calls so every constant scales automatically
// when the window is resized. Use local variables to cache in tight loops.
#define _SH GetScreenHeight()
#define _SW GetScreenWidth()

// ── Focus / text helpers
#define COLOR_FOCUS(c) (c ? YELLOW : GRAY)
#define COLOR_TEXT GRAY

#define CHAR_WIDTH(h)        MeasureText("W", int(0.8f * (h)))
#define TEXT_PAD(h)          CHAR_WIDTH(h)
#define TEXT_WIDTH(text, h)  MeasureText(text, int(0.8f * (h)))
#define CLIP_WIDTH(text, w, h) \
  (TEXT_WIDTH(text, h) - ((w) - TEXT_PAD(h) * 4)) / CHAR_WIDTH(h)
#define ALLIGN_RIGHT(text, w, h) \
  ((w) - TEXT_WIDTH((text), h) - TEXT_PAD(h) * 8)

// Legacy proportional sizes kept for TextInput / Header (panel-height relative)
#define TITLE_FONT_SIZE int(0.06f * GetScreenHeight())
#define TEXT_FONT_SIZE  int(0.04f * GetScreenHeight())

// ── Background colors
#define COLOR_BG_DARK         Color{10, 10, 20, 255}
#define COLOR_BG_MAIN         Color{15, 15, 20, 255}
#define COLOR_HEADER_BG       Color{30, 30, 40, 255}
#define COLOR_FOOTER_BG       Color{25, 25, 35, 255}

// ── UI element colors
#define COLOR_STAT_TEXT       Color{160, 160, 180, 255}
#define COLOR_DESC_TEXT       Color{200, 200, 200, 255}
#define COLOR_PROGRESS_BG     Color{40, 40, 40, 255}
#define COLOR_SCROLLBAR_BG    Color{40, 40, 60, 255}
#define COLOR_SCROLLBAR_THUMB Color{160, 160, 200, 200}

// ── Card type stat colors
#define COLOR_MONSTER_STAT    Color{180, 60, 60, 255}
#define COLOR_SPELL_STAT      Color{0, 180, 140, 255}
#define COLOR_TRAP_STAT       Color{160, 60, 200, 255}

// ── Alpha transparency
#define CARD_TYPE_ALPHA    90
#define CARD_PREVIEW_ALPHA 60
#define OVERLAY_FADE       0.35f

// ────────────────────────────────────────────────────────────
// All size / position values below are percentages of the
// current screen height (_SH) or width (_SW).
// Reference resolution: 1280 × 720.  Approximate px shown.
// ────────────────────────────────────────────────────────────

// ── Font sizes  (% of screen height)
#define FONT_MAIN_TITLE   int(0.067f * _SH)   // ~48px
#define FONT_MENU_ITEM    int(0.039f * _SH)   // ~28px
#define FONT_SCREEN_TITLE int(0.025f * _SH)   // ~18px
#define FONT_PANEL_TITLE  int(0.023f * _SH)   // ~16px
#define FONT_DECK_STATS   int(0.021f * _SH)   // ~15px
#define FONT_CARD_NAME    int(0.020f * _SH)   // ~14px
#define FONT_CARD_TYPE    int(0.0181f * _SH)  // ~13px
#define FONT_CARD_STAT    int(0.017f * _SH)   // ~12px
#define FONT_HELP_TEXT    int(0.0153f * _SH)  // ~11px
#define FONT_HELP_SMALL   int(0.023f * _SH)   // ~16px

// Panel-relative font / bar sizes (existing API, argument = panel height)
#define PANEL_HEADER_FONT(h)  int(0.03f * (h))
#define SEARCH_BAR_HEIGHT(h)  int(0.05f * (h))

// ── Screen chrome
#define HEADER_HEIGHT       int(0.039f * _SH)   // ~28px
#define HEADER_TITLE_X      int(0.008f * _SW)   // ~10px
#define HEADER_TITLE_Y      int(0.009f * _SH)   // ~6px
#define HELP_TEXT_X_OFFSET  int(0.125f * _SW)   // ~160px
#define HELP_TEXT_Y         int(0.012f * _SH)   // ~8px
#define MAIN_PAD_X          int(0.0047f * _SW)  // ~6px
#define MAIN_PAD_Y          int(0.045f * _SH)   // ~32px
#define MAIN_PAD_BOTTOM     int(0.067f * _SH)   // ~48px
#define STATUS_BAR_Y_OFFSET int(0.006f * _SH)   // ~4px

// ── Panel layout (dimensionless, percentages of screen width)
#define POOL_WIDTH_PERCENT    38
#define PREVIEW_WIDTH_PERCENT 24

// ── Card list / thumbnail
#define CARD_ITEM_HEIGHT       int(0.125f * _SH)   // ~90px
#define THUMBNAIL_WIDTH        int(0.081f * _SH)   // ~58px
#define THUMBNAIL_PAD          int(0.006f * _SH)   // ~4px
#define THUMBNAIL_TEXT_GAP     int(0.009f * _SH)   // ~6px
#define SEARCH_BAR_Y_OFFSET    int(0.039f * _SH)   // ~28px
#define LIST_Y_OFFSET          int(0.050f * _SH)   // ~36px
#define CARD_TYPE_Y            int(0.009f * _SH)   // ~6px
#define CARD_STAT_Y            int(0.034f * _SH)   // ~24px
#define CARD_NAME_Y            int(0.062f * _SH)   // ~44px
#define CARD_NAME_RIGHT_MARGIN int(0.027f * _SW)   // ~34px
#define COPY_COUNT_X_OFFSET    int(0.027f * _SW)   // ~34px
#define SELECTION_BORDER       int(0.003f * _SH + 0.5f) // ~2px
#define DECK_LIST_Y_OFFSET     int(0.123f * _SH)   // ~88px

// ── Scrollbar
#define SCROLLBAR_WIDTH     int(0.0025f * _SW)  // ~3px
#define SCROLLBAR_X_OFFSET  int(0.004f * _SW)   // ~5px
#define SCROLLBAR_H_TRIM    int(0.084f * _SH)   // ~60px
#define SCROLLBAR_THUMB_MIN int(0.012f * _SH)   // ~8px

// ── Preview panel
#define PREVIEW_PAD_X        int(0.006f * _SW)   // ~8px
#define PREVIEW_ART_Y_OFFSET int(0.039f * _SH)   // ~28px
#define PREVIEW_ART_SIDE_PAD int(0.013f * _SW)   // ~16px
#define PREVIEW_ASPECT_RATIO 1.45f
#define PREVIEW_INFO_GAP     int(0.009f * _SH)   // ~6px
#define PREVIEW_DESC_CHAR_W  (FONT_CARD_STAT * 2 / 3)   // ~8px; scales with font
#define PREVIEW_DESC_SIDE_PAD int(0.013f * _SW)  // ~16px

// ── Deck stats label x-offsets
#define STAT_SPL_X_OFFSET int(0.043f * _SW)   // ~55px
#define STAT_TRP_X_OFFSET int(0.086f * _SW)   // ~110px

// ── Main menu
#define MENU_ARROW_OFFSET       int(0.019f * _SW)  // ~24px
#define MENU_ITEM_SPACING       int(0.056f * _SH)  // ~40px
#define MENU_HELP_BOTTOM_OFFSET int(0.042f * _SH)  // ~30px

// ── Grid view — card sizes are derived from the panel width at draw time,
// so they automatically fill the available space.
#define GRID_COLS           4
#define GRID_GAP            int(0.0047f * _SW)    // ~6px
#define GRID_LABEL_H        int(0.023f * _SH)     // ~16px
#define GRID_CARD_W(panelW) (((panelW) - GRID_GAP * (GRID_COLS + 1)) / GRID_COLS)
#define GRID_CARD_H(panelW) (int((float)GRID_CARD_W(panelW) * PREVIEW_ASPECT_RATIO))
#define GRID_CELL_H(panelW) (GRID_CARD_H(panelW) + GRID_LABEL_H + GRID_GAP)
