#pragma once
// ── act/Summon — the summon composites (guards + Move + turn-state stamps) ──
// Each summon is the same three-part shape: legality (Query.hpp), materials
// (Move.hpp destruction), placement (SummonToMMZ), then flag stamps. Nothing
// here touches zones directly — if it moves a card, it does it via Move.hpp.
// Fusion lands in a Main Monster Zone (classic format: no Extra Monster Zone).
#include <vector>

#include "engine/action/Move.hpp"
#include "engine/action/Query.hpp"

namespace openjoey::engine::action {

using openjoey::ActionResult;

// Normal Summon (p.24): once per turn, Main Phase only, face-up ATK.
ActionResult SummonNormal(Duel &d, Card *c);

// Normal Set (p.24): once per turn, Main Phase only, face-down DEF.
ActionResult SummonSet(Duel &d, Card *c);

// Tribute Summon/Set (p.23): send N tributes to the Graveyard, then place.
ActionResult SummonTribute(Duel &d, Card *c, const std::vector<Card *> &tributes, bool faceDown);

// Flip Summon (p.25): your face-down Set monster -> face-up ATK; illegal the
// turn it was Set. The flipped card lands in d.pendingTriggers — whether it
// has an effect is a spec-provider concern, not the engine's (no hardcoding).
ActionResult FlipSummon(Duel &d, Card *c);

// Special Summon: from wherever the card sits, pose in the arguments.
enum class SpecialPose { Atk, DefUp, DefDown };
ActionResult SpecialSummon(Duel &d, Card *c, SpecialPose pose);
ActionResult SpecialSummon(Duel &d, Card *c, bool faceDown = false);

// Fusion Summon (p.20): Extra Deck -> Main Monster Zone; materials -> Graveyard.
ActionResult FusionSummon(Duel &d, Card *extra, const std::vector<Card *> &materials);

// Ritual Summon (p.21): hand -> MMZ; tribute levels >= ritual level.
ActionResult RitualSummon(Duel &d, Card *monster, const std::vector<Card *> &tributes);

// Token (rulebook tokens): engine-spawned, owned by the mat.
Card *SummonToken(Duel &d, const std::string &name, int atk, int def);

}  // namespace openjoey::engine::action
