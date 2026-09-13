

#pragma once

// ── ActionId (openjoey) ──────────────────────────────────────────────────────
// THE one action vocabulary: 213 actions, one per line, grouped by ruleset section.
// Player actions and card effects are the same kind of thing — an ActionSpec binds
// an ActionId to parameters, and the engine realizes EVERY id in
// duel/engine/Support.hpp perform() (exhaustive switch; the iterate-all-ids test
// proves nothing is unimplemented). Post-classic mechanics
// (Synchro/Xyz/Pendulum/Link) realize as classic-format gates.
//
// ABI: appended-only. Full per-action realization table:
// openjoey-engine/docs/ACTIONS.md.

#include <cstdint>

namespace openjoey {

enum class ActionId : uint16_t {
    None = 0,

    // ── ACTIVATION COSTS (paid before resolution, never refunded) ──
    Cost_Tribute,
    Cost_Discard,
    Cost_PayLP,
    Cost_BanishCost,

    // ── HAND / DECK MOVEMENTS ──
    Move_Draw,
    Move_MillToGY,
    Move_DiscardToGY,
    Move_ReturnHand,
    Move_ReturnDeck,
    Move_SearchToHand,
    Move_Excavate,

    // ── REMOVAL & DESTRUCTION ──
    Move_DestroyToGY,
    Move_SendToGY,
    Move_Banish,
    SendMaterialsToGraveyard,

    // ── SUMMONING (source -> monster zone / EMZ) ──
    Summon_Normal,
    Summon_Set,
    Summon_Flip,
    Summon_Special,
    Summon_Token,
    Summon_Fusion, // Classic Fusion Summon
    Summon_Synchro, // Classic Synchro Summon
    Summon_Xyz, // Classic Xyz Summon
    Summon_Ritual, // Classic Ritual Summon
    NormalSummon,
    NormalSet,
    PlayMonsterFaceUpAttack,
    PlayMonsterFaceDownDefense,
    TributeSummon,
    TributeSet,
    SendTributeToGraveyard,
    FlipSummon,
    FlipToFaceUpAttack,
    ActivateFlipEffectTrigger,
    SpecialSummon,
    SpecialSummonFromHand,
    SpecialSummonFromGraveyard,
    SpecialSummonFromBanished,
    SpecialSummonFromExtraDeck,
    SpecialSummonFaceUp,
    SpecialSummonFaceDown,
    ChooseAttackOrDefensePosition,
    SummonOrSetMonster,
    TributeForRitualSummon,

    // ── POSITION / VISIBILITY ──
    Pos_ChangeAToDef,
    Pos_ChangeDefToAtk,
    Pos_Flip,
    ChangeMonsterBattlePosition,
    ChangeToAttackPosition,
    ChangeToDefensePosition,
    CheckCannotChangePosition,

    // ── CHAIN & LIFE-POINT EFFECTS ──
    NegateActivation,
    NegateEffect,
    LP_Damage,
    LP_Gain,
    RespondWithEffect,
    AddToChain,
    PassChain,
    PassPriority,
    ResolveChain,
    BuildChain,
    ResolveInReverseOrder,

    // ── EQUIP ──
    Equip_Equip,
    Equip_Unequip,

    // ── COUNTERS ──
    Counter_Place,
    Counter_Remove,

    // ── DRAW PHASE ──
    DrawCard,
    SkipDraw,
    DrawFirstCard,
    Draw,

    // ── STANDBY PHASE ──
    EnterStandbyPhase,
    ResolveStandbyEffect,

    // ── MAIN PHASE 1 / 2 ──
    EnterMainPhase1,
    EnterMainPhase2,
    ActivateCardEffect,
    ActivateSpellEffect,
    ActivateTrapEffect,
    ActivateMonsterEffect,
    SetSpellCard,
    SetTrapCard,
    CheckMonsterPlayedThisTurn,
    CheckMonstersInExtraDeck,
    CheckMonstersInHand,
    CheckMaterialsInRequiredPlaces,
    MatchMaterialRequirements,
    PlaceInExtraMonsterZone,
    PlaceInExtraMonsterZoneOrPointedZone,
    PlaceInPointedZone,
    CannotPlayFaceUpDefense,
    CheckAlreadyChangedThisTurn,

    // ── END PHASE ──
    EnterEndPhase,
    SelectAndDiscard,
    AnnounceEndOfTurn,
    DiscardUntilHas6,
    CannotEndTurn,
    HandLimitUnresolved,
    ResolveEndPhaseEffects,

    // ── WIN CONDITIONS ──
    CheckWinConditions,
    DeckOut,
    CardEffectWin,
    ReduceLP0,
    Win,
    UnableToDraw,

    // ── TURN MANAGEMENT ──
    StartTurn,
    EndTurn,
    IncrementTurnNumber,
    ResetPerTurnState,
    SwapPlayers,
    FirstTurnSkips,
    StartingPlayerSkipBattle,
    StartingPlayerSkipDraw,

    // ── PUBLIC-ZONE ACTIONS ──
    ViewGraveyard,
    PickUpGraveyard,
    ZoneBecomesPendulumZone,

    // ── DECK MANAGEMENT ──
    ShuffleDeck,
    CutDeck,

    // ── NORMAL SUMMON / SET ──
    // (Already covered under Summoning section above)

    // ── TRIBUTE SUMMON / SET ──
    // (Already covered under Summoning section above)

    // ── FLIP SUMMON ──
    // (Already covered under Summoning section above)

    // ── SPECIAL SUMMON ──
    // (Already covered under Summoning section above)
    SummonFromCardEffect,

    // ── FUSION SUMMON ──
    ActivationFusionSummoningCard,
    PlaceFusionCardInSpellTrapZone,
    PlaceFusionMonsterInExtraMonsterZone,
    PlaceFusionSummoningCardInGraveyard,
    SendFusionMaterialsToGraveyard,
    CheckFusionMaterials,
    TakeFusionMonsterFromExtraDeck,

    // ── RITUAL SUMMON ──
    RitualSummon,
    ActivateRitualSpellCard,
    PlaceRitualSpellCardInGraveyard,
    PlayRitualMonsterInMainMonsterZone,
    HaveMatchingRitualMonster,
    HaveRitualSpellInHand,

    // ── SYNCHRO SUMMON (classic gate) ──
    CoLinked,
    CountLinkMonsterAs1OrLinkRating,
    DeclaresSynchroSummon,
    SendSynchroMaterialsToGraveyard,
    SumLevelsMustEqualSynchroLevel,
    SynchroSummon,
    TakeSynchroMonsterFromExtraDeck,
    CheckNonTunerMonsters,
    NeedOneTuner,
    CheckTunerMonster,

    // ── XYZ SUMMON (classic gate) ──
    ChooseXyzMonsterFromExtraDeck,
    DetachXyzMaterial,
    DeclareXyzSummoning,
    PlaceXyzMonsterOnTop,
    SendXyzMaterialToGraveyard,
    StackXyzMaterials,
    XyzSummon,
    CheckXyzMaterials,
    CheckXyzMaterialsFaceUp,

    // ── PENDULUM SUMMON (classic gate) ──
    ActivateInLeftmostZone,
    ActivateInRightmostZone,
    ActivatePendulumMonsterAsSpell,
    DeclarePendulumSummoning,
    PendulumMonsterGYToExtraDeck,
    PendulumSummon,
    HaveOnePendulumInEachZone,
    LevelsMustBeBetweenScales,
    CheckPendulumScales,

    // ── LINK SUMMON (classic gate) ──
    LinkArrowPointsToZone,
    LinkMaterialCanBeLinkMonster,
    LinkSummon,
    MonsterIsLinked,
    NearestPreviousLink,
    ResolveLinkFirst,
    ResolveLinkLast,
    TakeSynchroMonsterFromExtraDeck, // Duplicate entry, removed during cleanup
    // LinkSummon, // Duplicate entry, removed during cleanup

    // ── BATTLE PHASE ──
    EnterBattlePhase,
    SkipBattlePhase,
    SelectMonsterToAttackWith,
    SelectAttackTarget,
    DeclareAttack,
    AttackMonster,
    AttackDirectly,
    CanChooseNotToAttack,
    CancelAttack,
    ConfirmAttack,
    ReturnToMainPhase2,
    ConfirmAttackResolution,
    CanAttackDifferentMonster,
    CanAttackMultipleMonsters,
    CanAttackOnce,
    CanAttackSameMonster,
    CannotAttackAgain,
    CanCancelAttack,
    FaceUpAttackPosition,
    FirstMonsterStillConsideredAttacked,
    FirstPlayerCannotBattle,
    ProceedToDamageStep,
    MonsterRemovedBeforeDamageStep,
    NewMonsterPlayedBeforeDamageStep,
    CannotSkipIfMonsterOnField,
    CheckOpponentFieldEmpty,
    CheckDirectAttackLegal,
    CheckReplay,
    ReplayAfterFieldChange,
    HasNotAttackedYet,
    ReSelectNewTarget,

    // ── TRIBUTE SUMMON / SET (Requirements) ──
    Level5to6Need1,
    Level7orHigherNeed2,
    RequireTribute,
    SendTributedMonstersToGraveyard,
    HaveRequiredTribute,

    // ── END PHASE (Misc) ──
    CheckHandSize,
    MoreThan6Cards,

};
}  // namespace openjoey

