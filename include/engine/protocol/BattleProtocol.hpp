#pragma once
// ── BattleProtocol (openjoey::engine::protocol) ──────────────────────────────
// Engine-internal flow states: the sub-steps of the Battle Phase, the Damage
// Step, and chain resolution. These are *not* player actions (see
// ActionId.hpp) — the engine walks them automatically and records a trace
// the UI/tests can assert on. Naming is generic card-game vocabulary.

#include <cstdint>

namespace openjoey::engine::protocol {

// ── Battle Phase walk (declare → replay check → damage → return) ────────────
enum class BattleStep : uint8_t {
    Idle,            // no attack in progress
    AttackerChosen,  // SelectMonsterToAttackWith done
    TargetChosen,    // SelectAttackTarget done (or DirectDeclared)
    DirectDeclared,  // direct attack declared
    ReplayCheck,     // ConfirmAttack: legality re-validated
    DamageBegin,     // ProceedToDamageStep
    FlipRevealed,    // face-down defender turned face-up
    DamageCalculated,
    Resolved,   // battle applied (destruction + damage)
    Cancelled,  // player called the attack off / replay invalidated
};

// ── Damage Step sub-timing ───────────────────────────────────────────────────
enum class DamageStep : uint8_t {
    None,
    Flip,       // turn face-down monsters face-up; flip effects trigger
    Calculate,  // compare stats
    Compare,    // ATK-vs-ATK / ATK-vs-DEF branch chosen
    Apply,      // destruction + battle damage applied
    End,
};

// ── Damage outcomes (the Damage Step's result values) ────────────────────────
enum class DamageOutcome : uint8_t {
    None,
    DirectHit,                  // full ATK as damage, nothing destroyed
    AttackingMonsterATKHigher,  // defender destroyed + difference inflicted
    AttackingMonsterATKLower,   // attacker destroyed + rebound difference
    AttackingMonsterATKEqual,   // tie: both destroyed
    DefenderATKHigher,          // ATK vs ATK mirror (attacker destroyed)
    DefenseLower,               // ATK > DEF: defender destroyed, no damage
    DefenseHigher,              // ATK < DEF: rebound damage to attacker
    DefenseEqual,               // nothing happens
};

}  // namespace openjoey::engine::protocol