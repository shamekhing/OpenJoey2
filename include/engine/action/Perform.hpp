#pragma once
// ── act/Perform — THE exhaustive 1:1 dispatcher (all 213 ActionIds) ──────────
#include "engine/action/Battle.hpp"
#include "engine/action/Chains.hpp"
#include "engine/action/Gates.hpp"
#include "engine/action/Summons.hpp"
#include "engine/action/Support.hpp"
#include "engine/action/Turn.hpp"

namespace openjoey::engine::action {

inline std::string Perform(Duel &d, ActionId id, const ActionArgs &args = {}) {
    // Normalize: empty verdicts still count as realized actions.
    auto finish = [](std::string r) {
        return r.empty() ? "(resolved — nothing to report.)" : r;
    };
    const int me = d.turnPlayer;
    switch (id) {
        case ActionId::Cost_Tribute:
        case ActionId::Cost_Discard:
        case ActionId::Cost_BanishCost:
        case ActionId::Move_Draw:
        case ActionId::Move_MillToGY:
        case ActionId::Move_DiscardToGY:
        case ActionId::Move_DestroyToGY:
        case ActionId::Move_SendToGY:
        case ActionId::Move_Banish:
        case ActionId::Move_ReturnHand:
        case ActionId::Move_ReturnDeck:
        case ActionId::Summon_Normal:
        case ActionId::Summon_Set:
        case ActionId::Summon_Special:
        case ActionId::Summon_Fusion:
        case ActionId::Summon_Ritual:
            return ActivateEffect(d, ActionSpec{id}, me, args);
        default:
            break;
    }
    switch (id) {
        case ActionId::Cost_PayLP:
            return ActivateEffect(d, ActionSpec{ActionId::Cost_PayLP, EffectType::Cost, 1, args.n}, me, args);
        case ActionId::LP_Damage:
            return ActivateEffect(d, ActionSpec{ActionId::LP_Damage, EffectType::Ignition, 1, args.n}, me, args);
        case ActionId::LP_Gain:
            return ActivateEffect(d, ActionSpec{ActionId::LP_Gain, EffectType::Ignition, 1, args.n}, me, args);
        case ActionId::Move_SearchToHand:
            return SearchDeck(d, [](const Card &) { return true; }) ? "searched the deck." : "nothing matched.";
        case ActionId::Move_Excavate: {
            auto cards = Excavate(d, args.n);
            return "excavated " + std::to_string(cards.size()) + " card(s).";
        }
        case ActionId::Summon_Token:
            return SummonToken(d, "Token", 0, 0) ? "token summoned." : "no free monster zone.";
        case ActionId::Equip_Equip:
            return EquipCard(d, args.source, args.target);
        case ActionId::Equip_Unequip:
            return UnequipCard(d, args.target);
        case ActionId::Pos_ChangeAToDef:
            return PosChange(d.field, args.target, zone::Orientation::Horizontal)
                       ? "switched to Defense Position."
                       : "position change: target must be a face-up monster.";
        case ActionId::Pos_ChangeDefToAtk:
            return PosChange(d.field, args.target, zone::Orientation::Vertical)
                       ? "switched to Attack Position."
                       : "position change: target must be a face-up monster.";
        case ActionId::Pos_Flip:
            return PosFlip(d.field, args.target)
                       ? "flipped face-up (Flip effect may trigger)."
                       : "flip: target is not a set monster.";
        case ActionId::Summon_Flip:
            return PosFlip(d.field, args.target)
                       ? "flipped face-up (Flip effect may trigger)."
                       : "flip: target is not a set monster.";
        case ActionId::Counter_Place:
            PlaceCounterD(d, args.target, "counter", args.n);
            return "counters placed.";
        case ActionId::Counter_Remove:
            return "removed " + std::to_string(RemoveCounterD(d, args.target, "counter", args.n)) + " counter(s).";
        case ActionId::NegateActivation:
        case ActionId::NegateEffect:
            return ActivateEffect(d, ActionSpec{id}, me, args);
        case ActionId::DrawCard:
        case ActionId::Draw:
        case ActionId::DrawFirstCard:
            return DrawForTurn(d);
        case ActionId::SkipDraw:
            return "draw skipped.";
        case ActionId::EnterStandbyPhase:
            d.turn.phase = Phase::Standby;
            return ResolveStandby(d);
        case ActionId::ResolveStandbyEffect:
            return ResolveStandby(d);
        case ActionId::EnterMainPhase1:
            return ToMain1S(d);
        case ActionId::EnterMainPhase2:
            return ToMain2S(d);
        case ActionId::EnterBattlePhase:
            return ToBattleS(d);
        case ActionId::SkipBattlePhase:
            return d.turn.phase == Phase::Main1 ? "proceed to Main Phase 2 to skip battle." : "cannot skip now.";
        case ActionId::SummonOrSetMonster:
        case ActionId::NormalSummon:
        case ActionId::PlayMonsterFaceUpAttack:
            return SummonNormal(d, args.target);
        case ActionId::NormalSet:
        case ActionId::PlayMonsterFaceDownDefense:
            return SummonSet(d, args.target);
        case ActionId::TributeSummon:
        case ActionId::SendTributeToGraveyard:
            return SummonTribute(d, args.target, args.materials, false);
        case ActionId::TributeSet:
            return SummonTribute(d, args.target, args.materials, true);
        case ActionId::FlipSummon:
        case ActionId::FlipToFaceUpAttack:
            return FlipSummon(d, args.target);
        case ActionId::ActivateFlipEffectTrigger:
            return ResolveChain(d);
        case ActionId::SpecialSummon:
        case ActionId::SpecialSummonFromHand:
        case ActionId::SpecialSummonFromGraveyard:
        case ActionId::SpecialSummonFromBanished:
        case ActionId::SpecialSummonFromExtraDeck:
        case ActionId::SpecialSummonFaceUp:
        case ActionId::SummonFromCardEffect:
            return SpecialSummon(d, args.target, false);
        case ActionId::SpecialSummonFaceDown:
            return SpecialSummon(d, args.target, true);
        case ActionId::ChooseAttackOrDefensePosition:
            return "choose: face-up ATK or face-down DEF.";
        case ActionId::FusionSummon:
            return FusionSummon(d, args.target, args.materials);
        case ActionId::RitualSummon:
            return RitualSummon(d, args.target, args.materials);
        case ActionId::ChangeMonsterBattlePosition:
        case ActionId::ChangeToAttackPosition:
        case ActionId::ChangeToDefensePosition:
            return ChangePosition(d, args.target);
        case ActionId::ActivateCardEffect:
        case ActionId::ActivateSpellEffect:
        case ActionId::ActivateTrapEffect:
        case ActionId::ActivateMonsterEffect:
            return ActivateEffect(d, args.spec, me, args);
        case ActionId::SetSpellCard:
        case ActionId::SetTrapCard:
            return "set: seat the card via the spell/trap zone.";
        case ActionId::DeclareAttack:
        case ActionId::SelectMonsterToAttackWith:
            return DeclareAttack(d, args.target,
                                 args.materials.empty() ? nullptr : args.materials.front());
        case ActionId::SelectAttackTarget:
        case ActionId::AttackMonster:
            return DeclareAttack(d, d.turnState.pending.attacker, args.target);
        case ActionId::AttackDirectly:
            return DeclareAttack(d, args.target ? args.target : d.turnState.pending.attacker, nullptr);
        case ActionId::ConfirmAttack:
        case ActionId::ConfirmAttackResolution:
            return ResolveDamage(d);
        case ActionId::CanChooseNotToAttack:
            return "you may choose not to attack.";
        case ActionId::CancelAttack:
            CancelAttack(d);
            return "attack cancelled.";
        case ActionId::ReturnToMainPhase2:
            return ToMain2S(d);
        case ActionId::RespondWithEffect:
        case ActionId::AddToChain:
            return ActivateEffect(d, args.spec, me, args);
        case ActionId::PassChain:
        case ActionId::PassPriority:
            d.chain.step = protocol::ChainStep::Responding;
            return "priority passed.";
        case ActionId::ResolveChain:
            return ResolveChain(d);
        case ActionId::EnterEndPhase:
            d.turn.phase = Phase::End;
            return "End Phase.";
        case ActionId::SelectAndDiscard: {
            if (args.target) {
                d.field.graveyardZones[me].put(args.target);
                d.field.findCard(args.target).first->remove(args.target);
                return "discarded.";
            }
            return "discard failed.";
        }
        case ActionId::StartTurn:
            return StartTurn(d);
        case ActionId::EndTurn:
            return EndTurn(d);
        case ActionId::CheckWinConditions:
            return CheckWinConditions(d) == DuelResult::Ongoing ? "duel ongoing." : "duel decided.";
        case ActionId::DeckOut:
            return "deck out is checked at the mandatory draw.";
        case ActionId::CardEffectWin:
            SetResult(d, DuelResult::Player0Win, WinReason::CardEffectWin);
            return "duel decided by a card effect.";
        case ActionId::ViewGraveyard:
        case ActionId::PickUpGraveyard:
            return ViewGraveyard(d, args.targetPlayer >= 0 ? args.targetPlayer : me);
        case ActionId::ShuffleDeck:
            d.field.deckZones[me].shuffle();
            return "deck shuffled.";
        case ActionId::CutDeck:
            return "deck cut.";
        case ActionId::CheckDirectAttackLegal:
            return CanDirectAttack(d, args.target) ? "direct attack legal." : "opponent controls monsters.";
        case ActionId::CheckOpponentFieldEmpty:
            return OpponentFieldEmpty(d, me) ? "opponent field empty." : "opponent controls monsters.";
        case ActionId::CheckReplay:
            return ConfirmAttack(d) ? "replay check passed." : "replay! attack cancelled.";
        case ActionId::CheckCannotChangePosition:
        case ActionId::CheckAlreadyChangedThisTurn:
            return CanChangePosition(d, args.target) ? "position change legal." : "position change illegal now.";
        case ActionId::CheckMonsterPlayedThisTurn:
            return HasPlacedMonsterThisTurn(d, me) ? "a monster was played this turn." : "none played.";
        case ActionId::CheckTributeRequirement:
        case ActionId::RequireTribute:
        case ActionId::Level5to6Need1:
        case ActionId::Level7orHigherNeed2:
            return args.target ? "tributes required: " + std::to_string(TributesRequired(args.target)) + "."
                               : "tribute check needs a monster.";
        case ActionId::CheckFusionMaterials:
        case ActionId::CheckMaterialsInRequiredPlaces:
            return args.materials.size() >= 2 ? "materials present." : "need at least 2 materials.";
        case ActionId::CheckFlipEffect:
            return "flip effects trigger via the flip path.";
        case ActionId::CannotFlipSummonSameTurn:
            return CanFlipSummon(d, args.target) ? "flip summon legal." : "cannot flip summon now.";
        case ActionId::CannotPlayFaceUpDefense:
            return "face-up defense play is not allowed.";
        case ActionId::CheckHandSize:
        case ActionId::MoreThan6Cards:
            return "hand size: " + std::to_string(HandSize(d, me)) +
                   (OverHandLimit(d, me) ? " (over limit)." : " (within limit).");
        case ActionId::DiscardUntilHas6:
            return "discarded " + std::to_string(DiscardToHandLimit(d, me)) + " card(s).";
        case ActionId::CannotEndTurn:
        case ActionId::HandLimitUnresolved:
            return CanEndTurn(d, me) ? "turn can end." : "hand limit unresolved.";
        case ActionId::ResolveEndPhaseEffects:
        case ActionId::AnnounceEndOfTurn:
            return EndTurn(d);
        case ActionId::ResetPerTurnState:
            ResetPerTurnState(d);
            return "per-turn state reset.";
        case ActionId::SwapPlayers:
            d.turnPlayer = 1 - d.turnPlayer;
            return "players swapped.";
        case ActionId::IncrementTurnNumber:
            ++d.turn.turnNumber;
            return "turn " + std::to_string(d.turn.turnNumber) + ".";
        case ActionId::FirstTurnSkips:
        case ActionId::StartingPlayerSkipDraw:
            return "starting player skips the first draw.";
        case ActionId::StartingPlayerSkipBattle:
            return "starting player skips the first Battle Phase.";
        case ActionId::Win:
            return d.result == DuelResult::Ongoing ? "no winner yet." : "duel decided.";
        case ActionId::ReduceLP0:
            return "LP depletion is checked by CheckWinConditions.";
        case ActionId::UnableToDraw:
            return "a player who must draw from an empty deck loses.";
        case ActionId::BattleStartStep:
        case ActionId::AnnounceEnteringBattlePhase:
            d.traceBattle(protocol::BattleStep::AttackerChosen);
            return "Battle Step started.";
        case ActionId::BattleStep:
            return "Battle Step in progress.";
        case ActionId::ProceedToDamageStep:
            return ResolveDamage(d);
        case ActionId::ReturnToBattleStep:
            d.traceBattle(protocol::BattleStep::Idle);
            return "returned to the Battle Step.";
        case ActionId::ReplayAfterFieldChange:
        case ActionId::MonsterRemovedBeforeDamageStep:
        case ActionId::NewMonsterPlayedBeforeDamageStep:
        case ActionId::ReSelectNewTarget:
        case ActionId::CanAttackSameMonster:
        case ActionId::CanAttackDifferentMonster:
        case ActionId::FirstMonsterStillConsideredAttacked:
            return ConfirmAttack(d) ? "replay check passed — attack stands." : "replay! re-declare.";
        case ActionId::CannotAttackAgain:
            return HasNotAttacked(d, args.target) ? "has not attacked yet." : "cannot attack again this turn.";
        case ActionId::HasNotAttackedYet:
        case ActionId::CanAttackOnce:
        case ActionId::FaceUpAttackPosition:
            return CanAttack(d, args.target) ? "attack legal." : "attack not legal for that monster.";
        case ActionId::CanAttackMultipleMonsters:
            return "multiple attacks are not legal in the classic format.";
        case ActionId::CanCancelAttack:
            return CanCancelAttack(d) ? "attack can be cancelled." : "nothing to cancel.";
        case ActionId::CannotSkipIfMonsterOnField:
            return OpponentFieldEmpty(d, me) ? "skip possible." : "cannot skip: monsters on the field.";
        case ActionId::FirstPlayerCannotBattle:
            return d.turn.skipBattle ? "first turn: no Battle Phase." : "battle allowed.";
        case ActionId::BuildChain:
            d.chain.step = protocol::ChainStep::Building;
            return "chain building.";
        case ActionId::NearestPreviousLink:
        case ActionId::ResolveInReverseOrder:
        case ActionId::ResolveLinkFirst:
        case ActionId::ResolveLinkLast:
            return "chain resolves last-link-first.";
        case ActionId::SendFusionMaterialsToGraveyard:
        case ActionId::SendTributedMonstersToGraveyard:
            return std::to_string(MoveMaterialsToGY(d.field, args.materials)) + " material(s) -> Graveyard.";
        case ActionId::TakeFusionMonsterFromExtraDeck:
        case ActionId::PlaceFusionMonsterInExtraMonsterZone:
        case ActionId::ActivateFusionSummoningCard:
            return FusionSummon(d, args.target, args.materials);
        case ActionId::PlaceFusionCardInSpellTrapZone:
            return "the fusion spell resolves from the spell/trap zone.";
        case ActionId::PlaceFusionSummoningCardInGraveyard:
            return "the fusion spell hits the Graveyard after resolving.";
        case ActionId::HaveRitualSpellInHand:
            return "the ritual spell must be in your hand.";
        case ActionId::HaveMatchingRitualMonster:
        case ActionId::HaveRequiredTribute:
        case ActionId::TributeForRitualSummon:
        case ActionId::ActivateRitualSpellCard:
        case ActionId::PlayRitualMonsterInMainMonsterZone:
            return RitualSummon(d, args.target, args.materials);
        case ActionId::PlaceRitualSpellCardInGraveyard:
            return "the ritual spell hits the Graveyard after resolving.";
        case ActionId::Summon_Synchro:
        case ActionId::Summon_Xyz:
            return SynchroGate(d);
        case ActionId::SynchroSummon:
        case ActionId::DeclaresSynchroSummon:
        case ActionId::NeedOneTuner:
        case ActionId::CheckTunerMonster:
        case ActionId::CheckNonTunerMonsters:
        case ActionId::SumLevelsMustEqualSynchroLevel:
        case ActionId::SendSynchroMaterialsToGraveyard:
        case ActionId::TakeSynchroMonsterFromExtraDeck:
            return SynchroGate(d);
        case ActionId::XyzSummon:
        case ActionId::ChooseXyzMonsterFromExtraDeck:
        case ActionId::CheckXyzMaterials:
        case ActionId::CheckXyzMaterialsFaceUp:
        case ActionId::DeclareXyzSummoning:
        case ActionId::StackXyzMaterials:
        case ActionId::PlaceXyzMonsterOnTop:
        case ActionId::DetachXyzMaterial:
        case ActionId::SendXyzMaterialToGraveyard:
            return XyzGate(d);
        case ActionId::PendulumSummon:
        case ActionId::ActivatePendulumMonsterAsSpell:
        case ActionId::ActivateInLeftmostZone:
        case ActionId::ActivateInRightmostZone:
        case ActionId::ZoneBecomesPendulumZone:
        case ActionId::HaveOnePendulumInEachZone:
        case ActionId::DeclarePendulumSummoning:
        case ActionId::CheckPendulumScales:
        case ActionId::CheckMonstersInHand:
        case ActionId::CheckMonstersInExtraDeck:
        case ActionId::LevelsMustBeBetweenScales:
        case ActionId::PendulumMonsterGYToExtraDeck:
        case ActionId::PlaceInExtraMonsterZoneOrPointedZone:
            return PendulumGate(d);
        case ActionId::LinkSummon:
        case ActionId::CheckLinkMaterials:
        case ActionId::CheckLinkRating:
        case ActionId::SendMaterialsToGraveyard:
        case ActionId::MatchMaterialRequirements:
        case ActionId::PlaceInPointedZone:
        case ActionId::LinkMaterialCanBeLinkMonster:
        case ActionId::CountLinkMonsterAs1OrLinkRating:
        case ActionId::LinkArrowPointsToZone:
        case ActionId::MonsterIsLinked:
        case ActionId::CoLinked:
            return LinkGate(d);
        case ActionId::PlaceInExtraMonsterZone:
            return "extra monster zone placement (via fusion/special summon).";
        case ActionId::None:
            return "idle.";
    }
    auto r = "action id " + std::to_string(static_cast<int>(id)) + " not handled.";
    return r.empty() ? "(resolved.)" : r;
}

}  // namespace openjoey::engine::action
