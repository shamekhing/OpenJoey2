#pragma once
#include <cstdint>

namespace openjoey {

// ── Classic-Yu-Gi-Oh! effect catalog ────────────────────────────────────────
//
// Every entry below is (per your insight) expressible as one or more cards
// being *moved from one zone to another*.  This enum is the contract between
// layer 1 (card/) and layer 3 (field/): a Card declares which EffectIDs it
// subscribes to (see card/CardEffect.hpp), and field/EffectResolver interprets
// each id as a concrete zone-to-zone transformation.
//
// Era policy: pre-GX + 5D's mechanics only.  Link / Pendulum / Rush
// (all post-GX) are intentionally absent.
enum class EffectID : uint16_t {
    None = 0,

    // ── activation costs (paid before resolution, never refunded — Rulebook p.53) ──
    Cost_Tribute,       // Tribute a monster you control to the Graveyard
    Cost_Discard,       // Discard N cards from hand to the Graveyard
    Cost_PayLP,         // Pay Life Points
    Cost_BanishCost,    // Banish a card as a cost (often face-down)

    // ── hand / deck movements ────────────────────────────────────────────────
    Move_Draw,          // Deck  -> Hand           (draw 1 card)
    Move_MillToGY,      // Deck  -> Graveyard      (mill / "send to GY")
    Move_DiscardToGY,   // Hand  -> Graveyard      (discard)
    Move_ReturnHand,    // anywhere -> Hand        (return / "add to hand")
    Move_ReturnDeck,    // anywhere -> Deck        (return / "shuffle back")

    // ── field -> graveyard ───────────────────────────────────────────────────
    Move_DestroyToGY,   // destroy: field -> Graveyard
    Move_SendToGY,      // generic send: field -> Graveyard (cost or effect)

    // ── removal ──────────────────────────────────────────────────────────────
    Move_Banish,        // anywhere -> Banished (+ face-up/face-down)

    // ── summoning (source -> Monster zone / Extra Monster Zone) ──────────────
    Summon_Normal,      // Hand -> Main Monster Zone, FaceUp ATK  (once/turn)
    Summon_Set,         // Hand -> Main Monster Zone, FaceDown DEF
    Summon_Flip,        // FaceDown DEF -> FaceUp  (triggers Flip effect)
    Summon_Special,     // Deck/GY/ED -> Monster/EMZ, FaceUp
    Summon_Fusion,      // Extra Deck -> EMZ, materials -> GY
    Summon_Synchro,     // Extra Deck -> EMZ, materials -> GY
    Summon_Xyz,         // Extra Deck -> EMZ, materials attached as Xyz Materials
    Summon_Ritual,      // hand/Extra Deck -> Main Monster Zone via Ritual Spell

    // ── position / visibility ────────────────────────────────────────────────
    Pos_ChangeAToDef,   // Attack -> Defense
    Pos_ChangeDefToAtk, // Defense -> Attack
    Pos_Flip,           // FaceDown -> FaceUp (reveals card)

    // ── chain interaction (Rulebook p.44 Spell Speed) ───────────────────────
    NegateActivation,
    NegateEffect,

    // ── life points (non-zone, but classic effects couple LP to cards) ──────
    LP_Damage,
    LP_Gain,
};

} // namespace openjoey
