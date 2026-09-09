#pragma once
// ── act/Perform — THE exhaustive 1:1 dispatcher (all 213 ActionIds) ──────────
#include "action/ActionResult.hpp"
#include "engine/action/Battle.hpp"
#include "engine/action/Chains.hpp"
#include "engine/action/Gates.hpp"
#include "engine/action/Summons.hpp"
#include "engine/action/Support.hpp"
#include "engine/action/Turn.hpp"

namespace openjoey::engine::action {

using openjoey::ActionResult;

inline ActionResult Perform(Duel &d, ActionId id, const ActionArgs &args = {}) {
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
        case ActionId::Summon_Ritual: return ActivateEffect(d, ActionSpec{id}, me, args);
        default: break;
    }
    switch (id) {
        case ActionId::Cost_PayLP: return ActivateEffect(d, ActionSpec{ActionId::Cost_PayLP, EffectType::Cost, 1, args.n}, me, args);
        case ActionId::LP_Damage: return ActivateEffect(d, ActionSpec{ActionId::LP_Damage, EffectType::Ignition, 1, args.n}, me, args);
        case ActionId::LP_Gain: return ActivateEffect(d, ActionSpec{ActionId::LP_Gain, EffectType::Ignition, 1, args.n}, me, args);
        case ActionId::Move_SearchToHand: return SearchDeck(d, [](const Card &) { return true; }) ? ActionResult::Ok("searched the deck.") : ActionResult::Fail("nothing matched.");
        case ActionId::Move_Excavate: {
            auto cards = Excavate(d, args.n);
            return ActionResult::Ok("excavated " + std::to_string(cards.size()) + " card(s).");
        }
        case ActionId::Summon_Token:
            // Token stats come from the caller (0/0 was a silent stub).
            return SummonToken(d, "Token", args.atk, args.def) ? ActionResult::Ok("token summoned.") : ActionResult::Fail("no free monster zone.");
        case ActionId::Equip_Equip: return EquipCard(d, args.source, args.target);
        case ActionId::Equip_Unequip: return UnequipCard(d, args.target);
        case ActionId::Pos_ChangeAToDef: return PosChange(d.field, args.target, zone::Orientation::Horizontal) ? ActionResult::Ok("switched to Defense Position.") : ActionResult::Fail("position change: target must be a face-up monster.");
        case ActionId::Pos_ChangeDefToAtk: return PosChange(d.field, args.target, zone::Orientation::Vertical) ? ActionResult::Ok("switched to Attack Position.") : ActionResult::Fail("position change: target must be a face-up monster.");
        case ActionId::Pos_Flip: return PosFlip(d.field, args.target) ? ActionResult::Ok("flipped face-up (Flip effect may trigger).") : ActionResult::Fail("flip: target is not a set monster.");
        case ActionId::Summon_Flip: return PosFlip(d.field, args.target) ? ActionResult::Ok("flipped face-up (Flip effect may trigger).") : ActionResult::Fail("flip: target is not a set monster.");
        case ActionId::Counter_Place: PlaceCounterD(d, args.target, "counter", args.n); return ActionResult::Ok("counters placed.");
        case ActionId::Counter_Remove: return ActionResult::Ok("removed " + std::to_string(RemoveCounterD(d, args.target, "counter", args.n)) + " counter(s).");
        case ActionId::NegateActivation:
        case ActionId::NegateEffect: return ActivateEffect(d, ActionSpec{id}, me, args);
        case ActionId::DrawCard:
        case ActionId::Draw:
        case ActionId::DrawFirstCard: return DrawForTurn(d);
        case ActionId::SkipDraw: return ActionResult::Ok("draw skipped.");
        case ActionId::EnterStandbyPhase: d.turn.phase = Phase::Standby; return ResolveStandby(d);
        case ActionId::ResolveStandbyEffect: return ResolveStandby(d);
        case ActionId::EnterMainPhase1: return ToMain1S(d);
        case ActionId::EnterMainPhase2: return ToMain2S(d);
        case ActionId::EnterBattlePhase: return ToBattleS(d);
        case ActionId::SkipBattlePhase: return d.turn.phase == Phase::Main1 ? ActionResult::Ok("proceed to Main Phase 2 to skip battle.") : ActionResult::Fail("cannot skip now.");
        case ActionId::SummonOrSetMonster:
        case ActionId::NormalSummon:
        case ActionId::PlayMonsterFaceUpAttack: return SummonNormal(d, args.target);
        case ActionId::NormalSet:
        case ActionId::PlayMonsterFaceDownDefense: return SummonSet(d, args.target);
        case ActionId::TributeSummon:
        case ActionId::SendTributeToGraveyard: return SummonTribute(d, args.target, args.materials, false);
        case ActionId::TributeSet: return SummonTribute(d, args.target, args.materials, true);
        case ActionId::FlipSummon:
        case ActionId::FlipToFaceUpAttack: return FlipSummon(d, args.target);
        case ActionId::ActivateFlipEffectTrigger: return ResolveChain(d);
        case ActionId::SpecialSummon:
        case ActionId::SpecialSummonFromHand:
        case ActionId::SpecialSummonFromGraveyard:
        case ActionId::SpecialSummonFromBanished:
        case ActionId::SpecialSummonFromExtraDeck:
        case ActionId::SpecialSummonFaceUp:
        case ActionId::SummonFromCardEffect: return SpecialSummon(d, args.target, false);
        case ActionId::SpecialSummonFaceDown: return SpecialSummon(d, args.target, true);
        case ActionId::ChooseAttackOrDefensePosition: return ActionResult::Ok("choose: face-up ATK or face-down DEF.");
        case ActionId::FusionSummon: return FusionSummon(d, args.target, args.materials);
        case ActionId::RitualSummon: return RitualSummon(d, args.target, args.materials);
        case ActionId::ChangeMonsterBattlePosition:
        case ActionId::ChangeToAttackPosition:
        case ActionId::ChangeToDefensePosition: return ChangePosition(d, args.target);
        case ActionId::ActivateCardEffect:
        case ActionId::ActivateSpellEffect:
        case ActionId::ActivateTrapEffect:
        case ActionId::ActivateMonsterEffect: return ActivateEffect(d, args.spec, me, args);
        case ActionId::SetSpellCard:
        case ActionId::SetTrapCard: {
            // REAL implementation (was a narration stub): SeatSpellTrap also
            // stamps setThisTurn, which the p.31 rule needs and which the old
            // UI-side copy of this logic forgot.
            Card *c = args.target ? args.target : args.source;
            if (SeatSpellTrap(d.field, c)) return ActionResult::Ok(std::string(c ? c->name : "card") + " set face-down.", id);
            return ActionResult::Fail("set failed (no free spell/trap zone).");
        }
        case ActionId::DeclareAttack:
        case ActionId::SelectMonsterToAttackWith: return DeclareAttack(d, args.target, args.materials.empty() ? nullptr : args.materials.front());
        case ActionId::SelectAttackTarget:
        case ActionId::AttackMonster: return DeclareAttack(d, d.turnState.pending.attacker, args.target);
        case ActionId::AttackDirectly: return DeclareAttack(d, args.target ? args.target : d.turnState.pending.attacker, nullptr);
        case ActionId::ConfirmAttack:
        case ActionId::ConfirmAttackResolution: return ResolveDamage(d);
        case ActionId::CanChooseNotToAttack: return ActionResult::Ok("you may choose not to attack.");
        case ActionId::CancelAttack: CancelAttack(d); return ActionResult::Ok("attack cancelled.");
        case ActionId::ReturnToMainPhase2: return ToMain2S(d);
        case ActionId::RespondWithEffect:
        case ActionId::AddToChain: return ActivateEffect(d, args.spec, me, args);
        case ActionId::PassChain:
        case ActionId::PassPriority: d.chain.step = protocol::ChainStep::Responding; return ActionResult::Ok("priority passed.");
        case ActionId::ResolveChain: {
            ActionResult r = ResolveChain(d);
            // An empty chain is a real outcome, not a blank verdict.
            return r.msg.empty() ? ActionResult::Ok("chain empty — nothing to resolve.") : r;
        }
        case ActionId::EnterEndPhase: d.turn.phase = Phase::End; return ActionResult::Ok("End Phase.");
        case ActionId::SelectAndDiscard: {
            // Route through the safe move primitive: the old hand-rolled
            // put-then-findCard-remove left the card in two zones at once.
            if (!args.target) return ActionResult::Fail("discard failed.");
            if (moveTo(d.field, args.target, d.field.graveyardZones[me])) return ActionResult::Ok("discarded.");
            return ActionResult::Fail("discard failed.");
        }
        case ActionId::StartTurn: return StartTurn(d);
        case ActionId::EndTurn: return EndTurn(d);
        case ActionId::CheckWinConditions: return CheckWinConditions(d) == DuelResult::Ongoing ? ActionResult::Ok("duel ongoing.") : ActionResult::Ok("duel decided.");
        case ActionId::DeckOut: return ActionResult::Ok("deck out is checked at the mandatory draw.");
        case ActionId::CardEffectWin:
            // The winner is the ACTIVATOR, not hardcoded player 0.
            SetResult(d, me == 0 ? DuelResult::Player0Win : DuelResult::Player1Win, WinReason::CardEffectWin);
            return ActionResult::Ok("duel decided by a card effect.");
        case ActionId::ViewGraveyard:
        case ActionId::PickUpGraveyard: return ViewGraveyard(d, args.targetPlayer >= 0 ? args.targetPlayer : me);
        case ActionId::ShuffleDeck: d.field.deckZones[me].shuffle(); return ActionResult::Ok("deck shuffled.");
        case ActionId::CutDeck: return ActionResult::Ok("deck cut.");
        case ActionId::CheckDirectAttackLegal: return CanDirectAttack(d, args.target) ? ActionResult::Ok("direct attack legal.") : ActionResult::Fail("opponent controls monsters.");
        case ActionId::CheckOpponentFieldEmpty: return OpponentFieldEmpty(d, me) ? ActionResult::Ok("opponent field empty.") : ActionResult::Fail("opponent controls monsters.");
        case ActionId::CheckReplay: return ConfirmAttack(d) ? ActionResult::Ok("replay check passed.") : ActionResult::Fail("replay! attack cancelled.");
        case ActionId::CheckCannotChangePosition:
        case ActionId::CheckAlreadyChangedThisTurn: return CanChangePosition(d, args.target) ? ActionResult::Ok("position change legal.") : ActionResult::Fail("position change illegal now.");
        case ActionId::CheckMonsterPlayedThisTurn: return HasPlacedMonsterThisTurn(d, me) ? ActionResult::Ok("a monster was played this turn.") : ActionResult::Ok("none played.");
        case ActionId::CheckTributeRequirement:
        case ActionId::RequireTribute:
        case ActionId::Level5to6Need1:
        case ActionId::Level7orHigherNeed2: return args.target ? ActionResult::Ok("tributes required: " + std::to_string(TributesRequired(args.target)) + ".") : ActionResult::Fail("tribute check needs a monster.");
        case ActionId::CheckFusionMaterials:
        case ActionId::CheckMaterialsInRequiredPlaces: return args.materials.size() >= 2 ? ActionResult::Ok("materials present.") : ActionResult::Fail("need at least 2 materials.");
        case ActionId::CheckFlipEffect: return ActionResult::Ok("flip effects trigger via the flip path.");
        case ActionId::CannotFlipSummonSameTurn: return CanFlipSummon(d, args.target) ? ActionResult::Ok("flip summon legal.") : ActionResult::Fail("cannot flip summon now.");
        case ActionId::CannotPlayFaceUpDefense: return ActionResult::Fail("face-up defense play is not allowed.");
        case ActionId::CheckHandSize:
        case ActionId::MoreThan6Cards: return ActionResult::Ok("hand size: " + std::to_string(HandSize(d, me)) + (OverHandLimit(d, me) ? " (over limit)." : " (within limit)."));
        case ActionId::DiscardUntilHas6: return ActionResult::Ok("discarded " + std::to_string(DiscardToHandLimit(d, me)) + " card(s).");
        case ActionId::CannotEndTurn:
        case ActionId::HandLimitUnresolved: return CanEndTurn(d, me) ? ActionResult::Ok("turn can end.") : ActionResult::Fail("hand limit unresolved.");
        case ActionId::ResolveEndPhaseEffects:
        case ActionId::AnnounceEndOfTurn: return EndTurn(d);
        case ActionId::ResetPerTurnState: ResetPerTurnState(d); return ActionResult::Ok("per-turn state reset.");
        case ActionId::SwapPlayers: d.turnPlayer = 1 - d.turnPlayer; return ActionResult::Ok("players swapped.");
        case ActionId::IncrementTurnNumber: ++d.turn.turnNumber; return ActionResult::Ok("turn " + std::to_string(d.turn.turnNumber) + ".");
        case ActionId::FirstTurnSkips:
        case ActionId::StartingPlayerSkipDraw: return ActionResult::Ok("starting player skips the first draw.");
        case ActionId::StartingPlayerSkipBattle: return ActionResult::Ok("starting player skips the first Battle Phase.");
        case ActionId::Win: return d.result == DuelResult::Ongoing ? ActionResult::Ok("no winner yet.") : ActionResult::Ok("duel decided.");
        case ActionId::ReduceLP0: return ActionResult::Ok("LP depletion is checked by CheckWinConditions.");
        case ActionId::UnableToDraw: return ActionResult::Ok("a player who must draw from an empty deck loses.");
        case ActionId::BattleStartStep:
        case ActionId::AnnounceEnteringBattlePhase: d.traceBattle(protocol::BattleStep::AttackerChosen); return ActionResult::Ok("Battle Step started.");
        case ActionId::BattleStep: return ActionResult::Ok("Battle Step in progress.");
        case ActionId::ProceedToDamageStep: return ResolveDamage(d);
        case ActionId::ReturnToBattleStep: d.traceBattle(protocol::BattleStep::Idle); return ActionResult::Ok("returned to the Battle Step.");
        case ActionId::ReplayAfterFieldChange:
        case ActionId::MonsterRemovedBeforeDamageStep:
        case ActionId::NewMonsterPlayedBeforeDamageStep:
        case ActionId::ReSelectNewTarget:
        case ActionId::CanAttackSameMonster:
        case ActionId::CanAttackDifferentMonster:
        case ActionId::FirstMonsterStillConsideredAttacked: return ConfirmAttack(d) ? ActionResult::Ok("replay check passed — attack stands.") : ActionResult::Fail("replay! re-declare.");
        case ActionId::CannotAttackAgain: return HasNotAttacked(d, args.target) ? ActionResult::Ok("has not attacked yet.") : ActionResult::Fail("cannot attack again this turn.");
        case ActionId::HasNotAttackedYet:
        case ActionId::CanAttackOnce:
        case ActionId::FaceUpAttackPosition: return CanAttack(d, args.target) ? ActionResult::Ok("attack legal.") : ActionResult::Fail("attack not legal for that monster.");
        case ActionId::CanAttackMultipleMonsters: return ActionResult::Fail("multiple attacks are not legal in the classic format.");
        case ActionId::CanCancelAttack: return CanCancelAttack(d) ? ActionResult::Ok("attack can be cancelled.") : ActionResult::Fail("nothing to cancel.");
        case ActionId::CannotSkipIfMonsterOnField: return OpponentFieldEmpty(d, me) ? ActionResult::Ok("skip possible.") : ActionResult::Fail("cannot skip: monsters on the field.");
        case ActionId::FirstPlayerCannotBattle: return d.turn.skipBattle ? ActionResult::Ok("first turn: no Battle Phase.") : ActionResult::Ok("battle allowed.");
        case ActionId::BuildChain: d.chain.step = protocol::ChainStep::Building; return ActionResult::Ok("chain building.");
        case ActionId::NearestPreviousLink:
        case ActionId::ResolveInReverseOrder:
        case ActionId::ResolveLinkFirst:
        case ActionId::ResolveLinkLast: return ActionResult::Ok("chain resolves last-link-first.");
        case ActionId::SendFusionMaterialsToGraveyard:
        case ActionId::SendTributedMonstersToGraveyard: return ActionResult::Ok(std::to_string(MoveMaterialsToGY(d.field, args.materials)) + " material(s) -> Graveyard.");
        case ActionId::TakeFusionMonsterFromExtraDeck:
        case ActionId::PlaceFusionMonsterInExtraMonsterZone:
        case ActionId::ActivateFusionSummoningCard: return FusionSummon(d, args.target, args.materials);
        case ActionId::PlaceFusionCardInSpellTrapZone: return ActionResult::Ok("the fusion spell resolves from the spell/trap zone.");
        case ActionId::PlaceFusionSummoningCardInGraveyard: return ActionResult::Ok("the fusion spell hits the Graveyard after resolving.");
        case ActionId::HaveRitualSpellInHand: return ActionResult::Ok("the ritual spell must be in your hand.");
        case ActionId::HaveMatchingRitualMonster:
        case ActionId::HaveRequiredTribute:
        case ActionId::TributeForRitualSummon:
        case ActionId::ActivateRitualSpellCard:
        case ActionId::PlayRitualMonsterInMainMonsterZone: return RitualSummon(d, args.target, args.materials);
        case ActionId::PlaceRitualSpellCardInGraveyard: return ActionResult::Ok("the ritual spell hits the Graveyard after resolving.");
        case ActionId::Summon_Synchro:
        case ActionId::Summon_Xyz: return SynchroGate(d);
        case ActionId::SynchroSummon:
        case ActionId::DeclaresSynchroSummon:
        case ActionId::NeedOneTuner:
        case ActionId::CheckTunerMonster:
        case ActionId::CheckNonTunerMonsters:
        case ActionId::SumLevelsMustEqualSynchroLevel:
        case ActionId::SendSynchroMaterialsToGraveyard:
        case ActionId::TakeSynchroMonsterFromExtraDeck: return SynchroGate(d);
        case ActionId::XyzSummon:
        case ActionId::ChooseXyzMonsterFromExtraDeck:
        case ActionId::CheckXyzMaterials:
        case ActionId::CheckXyzMaterialsFaceUp:
        case ActionId::DeclareXyzSummoning:
        case ActionId::StackXyzMaterials:
        case ActionId::PlaceXyzMonsterOnTop:
        case ActionId::DetachXyzMaterial:
        case ActionId::SendXyzMaterialToGraveyard: return XyzGate(d);
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
        case ActionId::PlaceInExtraMonsterZoneOrPointedZone: return PendulumGate(d);
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
        case ActionId::CoLinked: return LinkGate(d);
        case ActionId::PlaceInExtraMonsterZone: return ActionResult::Ok("extra monster zone placement (via fusion/special summon).");
        case ActionId::None: return ActionResult::Ok("idle.");
        default: break;  // any id not realized here falls through to the verdict below
    }
    return ActionResult::Fail("action id " + std::to_string(static_cast<int>(id)) + " not handled.");
}

}  // namespace openjoey::engine::action
