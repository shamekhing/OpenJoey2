#pragma once
// ── ActionId (openjoey) ──────────────────────────────────────────────────────
// THE one action vocabulary: 213 actions, one per line, grouped by ruleset
// section. Player actions and card effects are the same kind of thing — an
// ActionSpec binds an ActionId to parameters, and the engine realizes EVERY
// id in duel/engine/Support.hpp perform() (exhaustive switch; the iterate-
// all-ids test proves nothing is unimplemented). Post-classic mechanics
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

    // ── REMOVAL & DESTRUCTION ──
    Move_DestroyToGY,
    Move_SendToGY,
    Move_Banish,

    // ── SUMMONING (source -> monster zone / EMZ) ──
    Summon_Normal,
    Summon_Set,
    Summon_Flip,
    Summon_Special,
    Summon_Token,
    Summon_Fusion,
    Summon_Synchro,
    Summon_Xyz,
    Summon_Ritual,

    // ── POSITION / VISIBILITY ──
    Pos_ChangeAToDef,
    Pos_ChangeDefToAtk,
    Pos_Flip,

    // ── CHAIN & LIFE-POINT EFFECTS ──
    NegateActivation,
    NegateEffect,
    LP_Damage,
    LP_Gain,

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

    // ── STANDBY PHASE ──
    EnterStandbyPhase,
    ResolveStandbyEffect,

    // ── MAIN PHASE 1 / 2 ──
    EnterMainPhase1,
    EnterMainPhase2,
    SummonOrSetMonster,
    ChangeMonsterBattlePosition,
    ChangeToAttackPosition,
    ChangeToDefensePosition,
    ActivateCardEffect,
    ActivateSpellEffect,
    ActivateTrapEffect,
    ActivateMonsterEffect,
    SetSpellCard,
    SetTrapCard,

    // ── NORMAL SUMMON / SET ──
    NormalSummon,
    NormalSet,
    PlayMonsterFaceUpAttack,
    PlayMonsterFaceDownDefense,

    // ── TRIBUTE SUMMON / SET ──
    TributeSummon,
    TributeSet,
    SendTributeToGraveyard,

    // ── FLIP SUMMON ──
    FlipSummon,
    FlipToFaceUpAttack,
    ActivateFlipEffectTrigger,

    // ── SPECIAL SUMMON ──
    SpecialSummon,
    SpecialSummonFromHand,
    SpecialSummonFromGraveyard,
    SpecialSummonFromBanished,
    SpecialSummonFromExtraDeck,
    SpecialSummonFaceUp,
    SpecialSummonFaceDown,
    ChooseAttackOrDefensePosition,

    // ── FUSION SUMMON ──
    FusionSummon,

    // ── RITUAL SUMMON ──
    RitualSummon,

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

    // ── CHAINING & PRIORITY ──
    RespondWithEffect,
    AddToChain,
    PassChain,
    PassPriority,
    ResolveChain,

    // ── END PHASE ──
    EnterEndPhase,
    SelectAndDiscard,

    // ── TURN MANAGEMENT ──
    StartTurn,
    EndTurn,

    // ── WIN CONDITIONS ──
    CheckWinConditions,
    DeckOut,
    CardEffectWin,
    Draw,

    // ── PUBLIC-ZONE ACTIONS ──
    ViewGraveyard,
    PickUpGraveyard,

    // ── DECK MANAGEMENT ──
    ShuffleDeck,
    CutDeck,

    // ── HAND / DECK MOVEMENTS ──
    Move_SearchToHand,
    Move_Excavate,

    // ── FUSION SUMMON ──
    ActivateFusionSummoningCard,

    // ── PENDULUM SUMMON (classic gate) ──
    ActivateInLeftmostZone,
    ActivateInRightmostZone,
    ActivatePendulumMonsterAsSpell,

    // ── RITUAL SUMMON ──
    ActivateRitualSpellCard,

    // ── END PHASE ──
    AnnounceEndOfTurn,

    // ── BATTLE PHASE ──
    AnnounceEnteringBattlePhase,
    BattleStartStep,
    BattleStep,

    // ── CHAINING & PRIORITY ──
    BuildChain,

    // ── BATTLE PHASE ──
    CanAttackDifferentMonster,
    CanAttackMultipleMonsters,
    CanAttackOnce,
    CanAttackSameMonster,
    CanCancelAttack,
    CannotAttackAgain,

    // ── END PHASE ──
    CannotEndTurn,

    // ── FLIP SUMMON ──
    CannotFlipSummonSameTurn,

    // ── MAIN PHASE 1 / 2 ──
    CannotPlayFaceUpDefense,

    // ── BATTLE PHASE ──
    CannotSkipIfMonsterOnField,

    // ── MAIN PHASE 1 / 2 ──
    CheckAlreadyChangedThisTurn,
    CheckCannotChangePosition,

    // ── BATTLE PHASE ──
    CheckDirectAttackLegal,

    // ── FLIP SUMMON ──
    CheckFlipEffect,

    // ── FUSION SUMMON ──
    CheckFusionMaterials,

    // ── END PHASE ──
    CheckHandSize,

    // ── LINK SUMMON (classic gate) ──
    CheckLinkMaterials,
    CheckLinkRating,

    // ── MAIN PHASE 1 / 2 ──
    CheckMaterialsInRequiredPlaces,
    CheckMonsterPlayedThisTurn,
    CheckMonstersInExtraDeck,
    CheckMonstersInHand,

    // ── SYNCHRO SUMMON (classic gate) ──
    CheckNonTunerMonsters,

    // ── BATTLE PHASE ──
    CheckOpponentFieldEmpty,

    // ── PENDULUM SUMMON (classic gate) ──
    CheckPendulumScales,

    // ── BATTLE PHASE ──
    CheckReplay,

    // ── TRIBUTE SUMMON / SET ──
    CheckTributeRequirement,

    // ── SYNCHRO SUMMON (classic gate) ──
    CheckTunerMonster,

    // ── XYZ SUMMON (classic gate) ──
    CheckXyzMaterials,
    CheckXyzMaterialsFaceUp,
    ChooseXyzMonsterFromExtraDeck,

    // ── LINK SUMMON (classic gate) ──
    CoLinked,

    // ── BATTLE PHASE ──
    ConfirmAttackResolution,

    // ── LINK SUMMON (classic gate) ──
    CountLinkMonsterAs1OrLinkRating,

    // ── PENDULUM SUMMON (classic gate) ──
    DeclarePendulumSummoning,

    // ── XYZ SUMMON (classic gate) ──
    DeclareXyzSummoning,

    // ── SYNCHRO SUMMON (classic gate) ──
    DeclaresSynchroSummon,

    // ── XYZ SUMMON (classic gate) ──
    DetachXyzMaterial,

    // ── END PHASE ──
    DiscardUntilHas6,

    // ── BATTLE PHASE ──
    FaceUpAttackPosition,
    FirstMonsterStillConsideredAttacked,
    FirstPlayerCannotBattle,

    // ── TURN MANAGEMENT ──
    FirstTurnSkips,

    // ── END PHASE ──
    HandLimitUnresolved,

    // ── BATTLE PHASE ──
    HasNotAttackedYet,

    // ── RITUAL SUMMON ──
    HaveMatchingRitualMonster,

    // ── PENDULUM SUMMON (classic gate) ──
    HaveOnePendulumInEachZone,

    // ── TRIBUTE SUMMON / SET ──
    HaveRequiredTribute,

    // ── RITUAL SUMMON ──
    HaveRitualSpellInHand,

    // ── TURN MANAGEMENT ──
    IncrementTurnNumber,

    // ── TRIBUTE SUMMON / SET ──
    Level5to6Need1,
    Level7orHigherNeed2,

    // ── PENDULUM SUMMON (classic gate) ──
    LevelsMustBeBetweenScales,

    // ── LINK SUMMON (classic gate) ──
    LinkArrowPointsToZone,
    LinkMaterialCanBeLinkMonster,
    LinkSummon,

    // ── MAIN PHASE 1 / 2 ──
    MatchMaterialRequirements,

    // ── LINK SUMMON (classic gate) ──
    MonsterIsLinked,

    // ── BATTLE PHASE ──
    MonsterRemovedBeforeDamageStep,

    // ── END PHASE ──
    MoreThan6Cards,

    // ── LINK SUMMON (classic gate) ──
    NearestPreviousLink,

    // ── SYNCHRO SUMMON (classic gate) ──
    NeedOneTuner,

    // ── BATTLE PHASE ──
    NewMonsterPlayedBeforeDamageStep,

    // ── PENDULUM SUMMON (classic gate) ──
    PendulumMonsterGYToExtraDeck,
    PendulumSummon,

    // ── FUSION SUMMON ──
    PlaceFusionCardInSpellTrapZone,
    PlaceFusionMonsterInExtraMonsterZone,
    PlaceFusionSummoningCardInGraveyard,

    // ── MAIN PHASE 1 / 2 ──
    PlaceInExtraMonsterZone,

    // ── PENDULUM SUMMON (classic gate) ──
    PlaceInExtraMonsterZoneOrPointedZone,

    // ── LINK SUMMON (classic gate) ──
    PlaceInPointedZone,

    // ── RITUAL SUMMON ──
    PlaceRitualSpellCardInGraveyard,

    // ── XYZ SUMMON (classic gate) ──
    PlaceXyzMonsterOnTop,

    // ── RITUAL SUMMON ──
    PlayRitualMonsterInMainMonsterZone,

    // ── BATTLE PHASE ──
    ProceedToDamageStep,
    ReSelectNewTarget,

    // ── WIN CONDITIONS ──
    ReduceLP0,

    // ── BATTLE PHASE ──
    ReplayAfterFieldChange,

    // ── TRIBUTE SUMMON / SET ──
    RequireTribute,

    // ── TURN MANAGEMENT ──
    ResetPerTurnState,

    // ── END PHASE ──
    ResolveEndPhaseEffects,

    // ── CHAINING & PRIORITY ──
    ResolveInReverseOrder,

    // ── LINK SUMMON (classic gate) ──
    ResolveLinkFirst,
    ResolveLinkLast,

    // ── BATTLE PHASE ──
    ReturnToBattleStep,

    // ── FUSION SUMMON ──
    SendFusionMaterialsToGraveyard,

    // ── PUBLIC-ZONE ACTIONS ──
    SendMaterialsToGraveyard,

    // ── SYNCHRO SUMMON (classic gate) ──
    SendSynchroMaterialsToGraveyard,

    // ── TRIBUTE SUMMON / SET ──
    SendTributedMonstersToGraveyard,

    // ── XYZ SUMMON (classic gate) ──
    SendXyzMaterialToGraveyard,
    StackXyzMaterials,

    // ── TURN MANAGEMENT ──
    StartingPlayerSkipBattle,
    StartingPlayerSkipDraw,

    // ── SYNCHRO SUMMON (classic gate) ──
    SumLevelsMustEqualSynchroLevel,

    // ── SPECIAL SUMMON ──
    SummonFromCardEffect,

    // ── TURN MANAGEMENT ──
    SwapPlayers,

    // ── SYNCHRO SUMMON (classic gate) ──
    SynchroSummon,

    // ── FUSION SUMMON ──
    TakeFusionMonsterFromExtraDeck,

    // ── SYNCHRO SUMMON (classic gate) ──
    TakeSynchroMonsterFromExtraDeck,

    // ── TRIBUTE SUMMON / SET ──
    TributeForRitualSummon,

    // ── WIN CONDITIONS ──
    UnableToDraw,
    Win,

    // ── XYZ SUMMON (classic gate) ──
    XyzSummon,

    // ── PENDULUM SUMMON (classic gate) ──
    ZoneBecomesPendulumZone,
};

}  // namespace openjoey
