#pragma once

// ── ActionId (openjoey) ──────────────────────────────────────────────────────
// THE action vocabulary, organized by the classic ruleset sections.
// 47 ids in 13 categories. Two roles, one table:
//   * §1–§8  = the OP space: an `Action{op, scope, amount}` in an ActionSpec is
//              realized by the interpreter (engine/action/Action.hpp).
//   * §11,14,21 = the VERB space: player entry points the Engine facade dispatches.
// Dissolved sections keep their headers with a pointer to where their content
// went — the table stays a readable index of the rules text.

#include <cstdint>
#include <vector>

namespace openjoey {

enum class ActionId : uint16_t {
    None = 0,

    // ── 1. ACTIVATION COSTS (paid before resolution, never refunded) ──
    Cost_Tribute,
    Cost_Discard,
    Cost_PayLP,
    Cost_BanishCost,

    // ── 2. HAND / DECK MOVEMENTS ──
    Move_Draw,
    Move_MillToGY,
    Move_DiscardToGY,
    Move_ReturnHand,
    Move_ReturnDeck,
    Move_SearchToHand,
    Move_Excavate,

    // ── 3. REMOVAL & DESTRUCTION ──
    Move_DestroyToGY,
    Move_SendToGY,
    Move_Banish,

    // ── 4. SUMMONING ──
    Summon_Normal,
    Summon_Set,
    FlipSummon,
    Summon_Special,
    Summon_Token,
    Summon_Fusion,
    Summon_Ritual,
    TributeSummon,
    TributeSet,

    // ── 5. POSITION / VISIBILITY ──
    ChangeMonsterBattlePosition,

    // ── 6. CHAIN & LIFE-POINT EFFECTS ──
    NegateActivation,
    NegateEffect,
    LP_Damage,
    LP_Gain,
    PassChain,
    ResolveChain,

    // ── 7. EQUIP ──
    Equip_Equip,
    Equip_Unequip,

    // ── 8. COUNTERS ──
    Counter_Place,
    Counter_Remove,

    // ── 9. DRAW PHASE — dissolved: draws are §2 Move_Draw; turn-1 skip is config
    // ── 10. STANDBY PHASE — dissolved: ResolveStandby lives in action/Turn.hpp

    // ── 11. MAIN PHASE 1 / 2 ──
    EnterMainPhase1,
    EnterMainPhase2,
    ActivateCardEffect,
    SetSpellTrap,

    // ── 12. END PHASE — dissolved: hand limit + discard are EndTurn steps
    // ── 13. WIN CONDITIONS — dissolved: DuelResult / WinReason enums (duel/Duel.hpp)

    // ── 14. TURN MANAGEMENT ──
    StartTurn,
    EndTurn,

    // ── 15. PUBLIC-ZONE ACTIONS ──
    ViewGraveyard,

    // ── 16. DECK MANAGEMENT ──
    ShuffleDeck,

    // ── 19. FUSION SUMMON — dissolved: absorbed by Summon_Fusion
    // ── 20. RITUAL SUMMON — dissolved: absorbed by Summon_Ritual

    // ── 21. BATTLE PHASE ──
    EnterBattlePhase,
    DeclareAttack,
    CancelAttack,
    ConfirmAttack,
    ProceedToDamageStep,

    // ── 22. TRIBUTE REQUIREMENTS — dissolved: TributesRequired() query
    // ── 23. END PHASE (Misc) — dissolved: OverHandLimit() query
};

// ── Scope: who/what an Action hits without extra arguments ───────────────────
enum class Scope : uint8_t {
    None = 0,
    Target,          // one explicit card (ActionArgs::target)
    Activator,       // the acting player themself
    Opponent,        // the activator's opponent (player)
    OppMonsters,     // ALL monsters the opponent controls (Raigeki)
    AllMonsters,     // all monsters on both sides (Dark Hole)
    AllSpellsTraps,  // all Spells/Traps on both sides (Heavy Storm)
    OppAttackPos,    // opponent's Attack-Position monsters (Mirror Force)
    PerOppMonster,   // amount x each opponent monster (Just Desserts)
};

// ── Action: ONE mat operation, as data ───────────────────────────────────────
// An ActionId from §1–8 + who it hits + how much. A card doing several things
// is an ActionSpec listing several Actions, resolved in order by the
// interpreter (engine/action/Action.hpp) through the Move.hpp primitives.
struct Action {
    ActionId op = ActionId::None;
    Scope scope = Scope::None;
    int amount = 1;
    bool needsTarget = false;
};

}  // namespace openjoey
