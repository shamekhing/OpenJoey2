# Action vocabulary — full realization table

Every `ActionId` (include/action/ActionId.hpp) is realized
exactly once in `duel/engine/Support.hpp` → `Engine::perform()`. The
`perform-every-id` test iterates the whole enum and fails on any action
without a real verdict. Post-classic mechanics realize as classic-format
gates (by design). Generated from the source — regenerate by re-running
the extraction, not by hand.

| # | Action | Ruleset section | Realization (perform arm) |
|---:|---|---|---|
| 1 | `Cost_Tribute` | ACTIVATION COSTS (paid before resolution, never refunded) | (calls a flow method) |
| 2 | `Cost_Discard` | ACTIVATION COSTS (paid before resolution, never refunded) | (calls a flow method) |
| 3 | `Cost_PayLP` | ACTIVATION COSTS (paid before resolution, never refunded) | activateEffect(ActionSpec{id, EffectType::Cost, 1, args.n}, d_.turnPlayer, args) |
| 4 | `Cost_BanishCost` | ACTIVATION COSTS (paid before resolution, never refunded) | (calls a flow method) |
| 5 | `Move_Draw` | HAND / DECK MOVEMENTS | (calls a flow method) |
| 6 | `Move_MillToGY` | HAND / DECK MOVEMENTS | (calls a flow method) |
| 7 | `Move_DiscardToGY` | HAND / DECK MOVEMENTS | (calls a flow method) |
| 8 | `Move_ReturnHand` | HAND / DECK MOVEMENTS | (calls a flow method) |
| 9 | `Move_ReturnDeck` | HAND / DECK MOVEMENTS | (calls a flow method) |
| 10 | `Move_DestroyToGY` | REMOVAL & DESTRUCTION | (calls a flow method) |
| 11 | `Move_SendToGY` | REMOVAL & DESTRUCTION | (calls a flow method) |
| 12 | `Move_Banish` | REMOVAL & DESTRUCTION | (calls a flow method) |
| 13 | `Summon_Normal` | SUMMONING (source -> monster zone / EMZ) | (calls a flow method) |
| 14 | `Summon_Set` | SUMMONING (source -> monster zone / EMZ) | (calls a flow method) |
| 15 | `Summon_Flip` | SUMMONING (source -> monster zone / EMZ) | (calls a flow method) |
| 16 | `Summon_Special` | SUMMONING (source -> monster zone / EMZ) | (calls a flow method) |
| 17 | `Summon_Token` | SUMMONING (source -> monster zone / EMZ) | summonToken("Token", 0, 0) ? "token summoned." : "no free monster zone." |
| 18 | `Summon_Fusion` | SUMMONING (source -> monster zone / EMZ) | (calls a flow method) |
| 19 | `Summon_Synchro` | SUMMONING (source -> monster zone / EMZ) | (calls a flow method) |
| 20 | `Summon_Xyz` | SUMMONING (source -> monster zone / EMZ) | classicGate("Synchro/Xyz Summons") |
| 21 | `Summon_Ritual` | SUMMONING (source -> monster zone / EMZ) | (calls a flow method) |
| 22 | `Pos_ChangeAToDef` | POSITION / VISIBILITY | action::Pos_Change(d_.field, args.target, zone::Orientation::Horizontal) ? "switched to Defense Position." ... |
| 23 | `Pos_ChangeDefToAtk` | POSITION / VISIBILITY | action::Pos_Change(d_.field, args.target, zone::Orientation::Vertical) ? "switched to Attack Position." : "... |
| 24 | `Pos_Flip` | POSITION / VISIBILITY | action::Pos_Flip(d_.field, args.target) ? "flipped face-up." : "flip: target is not a set monster." |
| 25 | `NegateActivation` | CHAIN & LIFE-POINT EFFECTS | (calls a flow method) |
| 26 | `NegateEffect` | CHAIN & LIFE-POINT EFFECTS | activateEffect(ActionSpec{id}, d_.turnPlayer, args) |
| 27 | `LP_Damage` | CHAIN & LIFE-POINT EFFECTS | activateEffect(ActionSpec{id, EffectType::Ignition, 1, args.n}, d_.turnPlayer, args) |
| 28 | `LP_Gain` | CHAIN & LIFE-POINT EFFECTS | activateEffect(ActionSpec{id, EffectType::Ignition, 1, args.n}, d_.turnPlayer, args) |
| 29 | `Equip_Equip` | EQUIP | equipCard(args.target, args.source) |
| 30 | `Equip_Unequip` | EQUIP | unequipCard(args.target) |
| 31 | `Counter_Place` | COUNTERS | "place counters (placeCounter)." |
| 32 | `Counter_Remove` | COUNTERS | "remove counters (removeCounter)." |
| 33 | `DrawCard` | DRAW PHASE | (calls a flow method) |
| 34 | `SkipDraw` | DRAW PHASE | "draw skipped." |
| 35 | `DrawFirstCard` | DRAW PHASE | drawForTurn() |
| 36 | `EnterStandbyPhase` | STANDBY PHASE | resolveStandby() |
| 37 | `ResolveStandbyEffect` | STANDBY PHASE | resolveStandby() |
| 38 | `EnterMainPhase1` | MAIN PHASE 1 / 2 | toMain1() |
| 39 | `EnterMainPhase2` | MAIN PHASE 1 / 2 | toMain2() |
| 40 | `SummonOrSetMonster` | MAIN PHASE 1 / 2 | (calls a flow method) |
| 41 | `ChangeMonsterBattlePosition` | MAIN PHASE 1 / 2 | (calls a flow method) |
| 42 | `ChangeToAttackPosition` | MAIN PHASE 1 / 2 | (calls a flow method) |
| 43 | `ChangeToDefensePosition` | MAIN PHASE 1 / 2 | changePosition(args.target) |
| 44 | `ActivateCardEffect` | MAIN PHASE 1 / 2 | (calls a flow method) |
| 45 | `ActivateSpellEffect` | MAIN PHASE 1 / 2 | (calls a flow method) |
| 46 | `ActivateTrapEffect` | MAIN PHASE 1 / 2 | (calls a flow method) |
| 47 | `ActivateMonsterEffect` | MAIN PHASE 1 / 2 | activateEffect(args.spec.id != ActionId::None ? args.spec : findClassicEffect(args.target ? args.target->na... |
| 48 | `SetSpellCard` | MAIN PHASE 1 / 2 | (calls a flow method) |
| 49 | `SetTrapCard` | MAIN PHASE 1 / 2 | "set: seat the card via the spell/trap zone (setSpellTrap path)." |
| 50 | `NormalSummon` | NORMAL SUMMON / SET | (calls a flow method) |
| 51 | `NormalSet` | NORMAL SUMMON / SET | (calls a flow method) |
| 52 | `PlayMonsterFaceUpAttack` | NORMAL SUMMON / SET | normalSummon(args.target) |
| 53 | `PlayMonsterFaceDownDefense` | NORMAL SUMMON / SET | normalSet(args.target) |
| 54 | `TributeSummon` | TRIBUTE SUMMON / SET | (calls a flow method) |
| 55 | `TributeSet` | TRIBUTE SUMMON / SET | tributeSet(args.target, args.materials) |
| 56 | `SendTributeToGraveyard` | TRIBUTE SUMMON / SET | tributeSummon(args.target, args.materials) |
| 57 | `FlipSummon` | FLIP SUMMON | flipSummon(args.target) |
| 58 | `FlipToFaceUpAttack` | FLIP SUMMON | flipSummon(args.target) |
| 59 | `ActivateFlipEffectTrigger` | FLIP SUMMON | resolveChain() |
| 60 | `SpecialSummon` | SPECIAL SUMMON | (calls a flow method) |
| 61 | `SpecialSummonFromHand` | SPECIAL SUMMON | (calls a flow method) |
| 62 | `SpecialSummonFromGraveyard` | SPECIAL SUMMON | (calls a flow method) |
| 63 | `SpecialSummonFromBanished` | SPECIAL SUMMON | (calls a flow method) |
| 64 | `SpecialSummonFromExtraDeck` | SPECIAL SUMMON | (calls a flow method) |
| 65 | `SpecialSummonFaceUp` | SPECIAL SUMMON | (calls a flow method) |
| 66 | `SpecialSummonFaceDown` | SPECIAL SUMMON | specialSummon(args.target, /*faceDown=*/true) |
| 67 | `ChooseAttackOrDefensePosition` | SPECIAL SUMMON | "choose: face-up ATK (specialSummon) or face-down DEF." |
| 68 | `FusionSummon` | FUSION SUMMON | fusionSummon(args.target, args.materials) |
| 69 | `RitualSummon` | RITUAL SUMMON | ritualSummon(args.target, args.materials) |
| 70 | `EnterBattlePhase` | BATTLE PHASE | toBattle() |
| 71 | `SkipBattlePhase` | BATTLE PHASE | d_.turn.phase == Phase::Main1 ? "proceed to Main Phase 2 to skip battle." : "cannot skip now." |
| 72 | `SelectMonsterToAttackWith` | BATTLE PHASE | declareAttack(args.target, args.materials.empty() ? nullptr : args.materials.front()) |
| 73 | `SelectAttackTarget` | BATTLE PHASE | (calls a flow method) |
| 74 | `DeclareAttack` | BATTLE PHASE | (calls a flow method) |
| 75 | `AttackMonster` | BATTLE PHASE | declareAttack(pending_.attacker, args.target) |
| 76 | `AttackDirectly` | BATTLE PHASE | declareAttack(args.target ? args.target : pending_.attacker, nullptr) |
| 77 | `CanChooseNotToAttack` | BATTLE PHASE | "you may choose not to attack." |
| 78 | `CancelAttack` | BATTLE PHASE | "attack cancelled." |
| 79 | `ConfirmAttack` | BATTLE PHASE | resolveDamage() |
| 80 | `ReturnToMainPhase2` | BATTLE PHASE | toMain2() |
| 81 | `RespondWithEffect` | CHAINING & PRIORITY | (calls a flow method) |
| 82 | `AddToChain` | CHAINING & PRIORITY | activateEffect(args.spec, d_.turnPlayer, args) |
| 83 | `PassChain` | CHAINING & PRIORITY | (calls a flow method) |
| 84 | `PassPriority` | CHAINING & PRIORITY | "priority passed." |
| 85 | `ResolveChain` | CHAINING & PRIORITY | resolveChain() |
| 86 | `EnterEndPhase` | END PHASE | "End Phase." |
| 87 | `SelectAndDiscard` | END PHASE | action::detail::moveCard(d_.field, args.target, d_.field.graveyardZones[d_.turnPlayer]) ? "discarded." : "d... |
| 88 | `StartTurn` | TURN MANAGEMENT | startTurn() |
| 89 | `EndTurn` | TURN MANAGEMENT | endTurn() |
| 90 | `CheckWinConditions` | WIN CONDITIONS | checkWinConditions() == DuelResult::Ongoing ? "duel ongoing." : "duel decided." |
| 91 | `DeckOut` | WIN CONDITIONS | "deck out is checked at the mandatory draw." |
| 92 | `CardEffectWin` | WIN CONDITIONS | "duel decided by a card effect." |
| 93 | `Draw` | WIN CONDITIONS | drawForTurn() |
| 94 | `ViewGraveyard` | PUBLIC-ZONE ACTIONS | viewGraveyard(args.targetPlayer >= 0 ? args.targetPlayer : d_.turnPlayer) |
| 95 | `PickUpGraveyard` | PUBLIC-ZONE ACTIONS | viewGraveyard(args.targetPlayer >= 0 ? args.targetPlayer : d_.turnPlayer) |
| 96 | `ShuffleDeck` | DECK MANAGEMENT | "deck shuffled." |
| 97 | `CutDeck` | DECK MANAGEMENT | "deck cut." |
| 98 | `Move_SearchToHand` | HAND / DECK MOVEMENTS | searchDeck([](const Card &) { return true |
| 99 | `Move_Excavate` | HAND / DECK MOVEMENTS | "excavated " + std::to_string(cards.size()) + " card(s)." |
| 100 | `ActivateFusionSummoningCard` | FUSION SUMMON | fusionSummon(args.target, args.materials) |
| 101 | `ActivateInLeftmostZone` | PENDULUM SUMMON (classic gate) | (calls a flow method) |
| 102 | `ActivateInRightmostZone` | PENDULUM SUMMON (classic gate) | (calls a flow method) |
| 103 | `ActivatePendulumMonsterAsSpell` | PENDULUM SUMMON (classic gate) | (calls a flow method) |
| 104 | `ActivateRitualSpellCard` | RITUAL SUMMON | ritualSummon(args.target, args.materials) |
| 105 | `AnnounceEndOfTurn` | END PHASE | endTurn() |
| 106 | `AnnounceEnteringBattlePhase` | BATTLE PHASE | "Battle Step started." |
| 107 | `BattleStartStep` | BATTLE PHASE | (calls a flow method) |
| 108 | `BattleStep` | BATTLE PHASE | "Battle Step in progress." |
| 109 | `BuildChain` | CHAINING & PRIORITY | "chain building." |
| 110 | `CanAttackDifferentMonster` | BATTLE PHASE | (calls a flow method) |
| 111 | `CanAttackMultipleMonsters` | BATTLE PHASE | "multiple attacks are not legal in the classic format." |
| 112 | `CanAttackOnce` | BATTLE PHASE | (calls a flow method) |
| 113 | `CanAttackSameMonster` | BATTLE PHASE | (calls a flow method) |
| 114 | `CanCancelAttack` | BATTLE PHASE | canCancelAttack() ? "attack can be cancelled." : "nothing to cancel." |
| 115 | `CannotAttackAgain` | BATTLE PHASE | hasNotAttacked(args.target) ? "has not attacked yet." : "cannot attack again this turn." |
| 116 | `CannotEndTurn` | END PHASE | (calls a flow method) |
| 117 | `CannotFlipSummonSameTurn` | FLIP SUMMON | canFlipSummon(args.target) ? "flip summon legal." : "cannot flip summon now." |
| 118 | `CannotPlayFaceUpDefense` | MAIN PHASE 1 / 2 | "face-up defense play is not allowed." |
| 119 | `CannotSkipIfMonsterOnField` | BATTLE PHASE | opponentFieldEmpty() ? "skip possible." : "cannot skip: monsters on the field." |
| 120 | `CheckAlreadyChangedThisTurn` | MAIN PHASE 1 / 2 | canChangePosition(args.target) ? "position change legal." : "position change illegal now." |
| 121 | `CheckCannotChangePosition` | MAIN PHASE 1 / 2 | (calls a flow method) |
| 122 | `CheckDirectAttackLegal` | BATTLE PHASE | canDirectAttack(args.target) ? "direct attack legal." : "opponent controls monsters." |
| 123 | `CheckFlipEffect` | FLIP SUMMON | "flip effects trigger via the flip path." |
| 124 | `CheckFusionMaterials` | FUSION SUMMON | (calls a flow method) |
| 125 | `CheckHandSize` | END PHASE | (calls a flow method) |
| 126 | `CheckLinkMaterials` | LINK SUMMON (classic gate) | (calls a flow method) |
| 127 | `CheckLinkRating` | LINK SUMMON (classic gate) | (calls a flow method) |
| 128 | `CheckMaterialsInRequiredPlaces` | MAIN PHASE 1 / 2 | args.materials.size() >= 2 ? "materials present." : "need at least 2 materials." |
| 129 | `CheckMonsterPlayedThisTurn` | MAIN PHASE 1 / 2 | hasPlacedMonsterThisTurn(d_.turnPlayer) ? "a monster was played this turn." : "none played." |
| 130 | `CheckMonstersInExtraDeck` | MAIN PHASE 1 / 2 | (calls a flow method) |
| 131 | `CheckMonstersInHand` | MAIN PHASE 1 / 2 | (calls a flow method) |
| 132 | `CheckNonTunerMonsters` | SYNCHRO SUMMON (classic gate) | (calls a flow method) |
| 133 | `CheckOpponentFieldEmpty` | BATTLE PHASE | opponentFieldEmpty() ? "opponent field empty." : "opponent controls monsters." |
| 134 | `CheckPendulumScales` | PENDULUM SUMMON (classic gate) | (calls a flow method) |
| 135 | `CheckReplay` | BATTLE PHASE | checkReplay() ? "replay check passed." : "replay! attack cancelled." |
| 136 | `CheckTributeRequirement` | TRIBUTE SUMMON / SET | (calls a flow method) |
| 137 | `CheckTunerMonster` | SYNCHRO SUMMON (classic gate) | (calls a flow method) |
| 138 | `CheckXyzMaterials` | XYZ SUMMON (classic gate) | (calls a flow method) |
| 139 | `CheckXyzMaterialsFaceUp` | XYZ SUMMON (classic gate) | (calls a flow method) |
| 140 | `ChooseXyzMonsterFromExtraDeck` | XYZ SUMMON (classic gate) | (calls a flow method) |
| 141 | `CoLinked` | LINK SUMMON (classic gate) | classicGate("Link Summons") |
| 142 | `ConfirmAttackResolution` | BATTLE PHASE | resolveDamage() |
| 143 | `CountLinkMonsterAs1OrLinkRating` | LINK SUMMON (classic gate) | (calls a flow method) |
| 144 | `DeclarePendulumSummoning` | PENDULUM SUMMON (classic gate) | (calls a flow method) |
| 145 | `DeclareXyzSummoning` | XYZ SUMMON (classic gate) | (calls a flow method) |
| 146 | `DeclaresSynchroSummon` | SYNCHRO SUMMON (classic gate) | (calls a flow method) |
| 147 | `DetachXyzMaterial` | XYZ SUMMON (classic gate) | (calls a flow method) |
| 148 | `DiscardUntilHas6` | END PHASE | "discarded " + std::to_string(discardToHandLimit()) + " card(s)." |
| 149 | `FaceUpAttackPosition` | BATTLE PHASE | canAttack(args.target) ? "attack legal." : "attack not legal for that monster." |
| 150 | `FirstMonsterStillConsideredAttacked` | BATTLE PHASE | checkReplay() ? "replay check passed — attack stands." : "replay! re-declare." |
| 151 | `FirstPlayerCannotBattle` | BATTLE PHASE | d_.turn.skipBattle ? "first turn: no Battle Phase." : "battle allowed." |
| 152 | `FirstTurnSkips` | TURN MANAGEMENT | (calls a flow method) |
| 153 | `HandLimitUnresolved` | END PHASE | canEndTurn() ? "turn can end." : "hand limit unresolved." |
| 154 | `HasNotAttackedYet` | BATTLE PHASE | (calls a flow method) |
| 155 | `HaveMatchingRitualMonster` | RITUAL SUMMON | (calls a flow method) |
| 156 | `HaveOnePendulumInEachZone` | PENDULUM SUMMON (classic gate) | (calls a flow method) |
| 157 | `HaveRequiredTribute` | TRIBUTE SUMMON / SET | (calls a flow method) |
| 158 | `HaveRitualSpellInHand` | RITUAL SUMMON | "the ritual spell must be in your hand." |
| 159 | `IncrementTurnNumber` | TURN MANAGEMENT | "turn " + std::to_string(d_.turn.turnNumber) + "." |
| 160 | `Level5to6Need1` | TRIBUTE SUMMON / SET | (calls a flow method) |
| 161 | `Level7orHigherNeed2` | TRIBUTE SUMMON / SET | args.target ? "tributes required: " + std::to_string(DuelConfig::tributesFor(args.target->level)) + "." : "... |
| 162 | `LevelsMustBeBetweenScales` | PENDULUM SUMMON (classic gate) | (calls a flow method) |
| 163 | `LinkArrowPointsToZone` | LINK SUMMON (classic gate) | (calls a flow method) |
| 164 | `LinkMaterialCanBeLinkMonster` | LINK SUMMON (classic gate) | (calls a flow method) |
| 165 | `LinkSummon` | LINK SUMMON (classic gate) | (calls a flow method) |
| 166 | `MatchMaterialRequirements` | MAIN PHASE 1 / 2 | (calls a flow method) |
| 167 | `MonsterIsLinked` | LINK SUMMON (classic gate) | (calls a flow method) |
| 168 | `MonsterRemovedBeforeDamageStep` | BATTLE PHASE | (calls a flow method) |
| 169 | `MoreThan6Cards` | END PHASE | "hand size: " + std::to_string(handSize()) + (overHandLimit() ? " (over limit)." : " (within limit).") |
| 170 | `NearestPreviousLink` | LINK SUMMON (classic gate) | (calls a flow method) |
| 171 | `NeedOneTuner` | SYNCHRO SUMMON (classic gate) | (calls a flow method) |
| 172 | `NewMonsterPlayedBeforeDamageStep` | BATTLE PHASE | (calls a flow method) |
| 173 | `PendulumMonsterGYToExtraDeck` | PENDULUM SUMMON (classic gate) | (calls a flow method) |
| 174 | `PendulumSummon` | PENDULUM SUMMON (classic gate) | (calls a flow method) |
| 175 | `PlaceFusionCardInSpellTrapZone` | FUSION SUMMON | "the fusion spell resolves from the spell/trap zone." |
| 176 | `PlaceFusionMonsterInExtraMonsterZone` | FUSION SUMMON | fusionSummon(args.target, args.materials) |
| 177 | `PlaceFusionSummoningCardInGraveyard` | FUSION SUMMON | "the fusion spell hits the Graveyard after resolving." |
| 178 | `PlaceInExtraMonsterZone` | MAIN PHASE 1 / 2 | "extra monster zone placement (via fusion/special summon)." |
| 179 | `PlaceInExtraMonsterZoneOrPointedZone` | PENDULUM SUMMON (classic gate) | classicGate("Pendulum Summons") |
| 180 | `PlaceInPointedZone` | LINK SUMMON (classic gate) | (calls a flow method) |
| 181 | `PlaceRitualSpellCardInGraveyard` | RITUAL SUMMON | "the ritual spell hits the Graveyard after resolving." |
| 182 | `PlaceXyzMonsterOnTop` | XYZ SUMMON (classic gate) | (calls a flow method) |
| 183 | `PlayRitualMonsterInMainMonsterZone` | RITUAL SUMMON | ritualSummon(args.target, args.materials) |
| 184 | `ProceedToDamageStep` | BATTLE PHASE | resolveDamage() |
| 185 | `ReSelectNewTarget` | BATTLE PHASE | (calls a flow method) |
| 186 | `ReduceLP0` | WIN CONDITIONS | "LP depletion is checked by checkWinConditions." |
| 187 | `ReplayAfterFieldChange` | BATTLE PHASE | (calls a flow method) |
| 188 | `RequireTribute` | TRIBUTE SUMMON / SET | (calls a flow method) |
| 189 | `ResetPerTurnState` | TURN MANAGEMENT | "per-turn state reset." |
| 190 | `ResolveEndPhaseEffects` | END PHASE | (calls a flow method) |
| 191 | `ResolveInReverseOrder` | CHAINING & PRIORITY | (calls a flow method) |
| 192 | `ResolveLinkFirst` | LINK SUMMON (classic gate) | (calls a flow method) |
| 193 | `ResolveLinkLast` | LINK SUMMON (classic gate) | "chain resolves last-link-first." |
| 194 | `ReturnToBattleStep` | BATTLE PHASE | "returned to the Battle Step." |
| 195 | `SendFusionMaterialsToGraveyard` | FUSION SUMMON | (calls a flow method) |
| 196 | `SendMaterialsToGraveyard` | PUBLIC-ZONE ACTIONS | (calls a flow method) |
| 197 | `SendSynchroMaterialsToGraveyard` | SYNCHRO SUMMON (classic gate) | (calls a flow method) |
| 198 | `SendTributedMonstersToGraveyard` | TRIBUTE SUMMON / SET | std::to_string(action::Move_MaterialsToGY(d_.field, args.materials)) + " material(s) -> Graveyard." |
| 199 | `SendXyzMaterialToGraveyard` | XYZ SUMMON (classic gate) | classicGate("Xyz Summons") |
| 200 | `StackXyzMaterials` | XYZ SUMMON (classic gate) | (calls a flow method) |
| 201 | `StartingPlayerSkipBattle` | TURN MANAGEMENT | "starting player skips the first Battle Phase." |
| 202 | `StartingPlayerSkipDraw` | TURN MANAGEMENT | "starting player skips the first draw." |
| 203 | `SumLevelsMustEqualSynchroLevel` | SYNCHRO SUMMON (classic gate) | (calls a flow method) |
| 204 | `SummonFromCardEffect` | SPECIAL SUMMON | specialSummon(args.target, /*faceDown=*/false) |
| 205 | `SwapPlayers` | TURN MANAGEMENT | "players swapped." |
| 206 | `SynchroSummon` | SYNCHRO SUMMON (classic gate) | (calls a flow method) |
| 207 | `TakeFusionMonsterFromExtraDeck` | FUSION SUMMON | (calls a flow method) |
| 208 | `TakeSynchroMonsterFromExtraDeck` | SYNCHRO SUMMON (classic gate) | classicGate("Synchro Summons") |
| 209 | `TributeForRitualSummon` | TRIBUTE SUMMON / SET | ritualSummon(args.target, args.materials) |
| 210 | `UnableToDraw` | WIN CONDITIONS | "a player who must draw from an empty deck loses." |
| 211 | `Win` | WIN CONDITIONS | d_.result == DuelResult::Ongoing ? "no winner yet." : "duel decided." |
| 212 | `XyzSummon` | XYZ SUMMON (classic gate) | (calls a flow method) |
| 213 | `ZoneBecomesPendulumZone` | PENDULUM SUMMON (classic gate) | (calls a flow method) |
