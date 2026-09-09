#pragma once
// ── openjoey::ai — RESERVED for the two planned agents (not implemented yet) ─
// The architecture reserves this namespace and fixes its contracts today so
// the engine exposes the right seams. Both agents live in a future
// openjoey-ai repo depending on engine -> cards -> foundation; both stay
// raylib-free and headless-testable (they train/run headless).
//
// 1. openjoey::ai::player  — reinforcement-learning opponent.
//    Contract it consumes (already exposed by openjoey::engine):
//      * Engine::observe(viewer)      -> read-only StateView snapshot
//      * Engine::legalActions(player) -> std::vector<ActionSpec> (action space)
//      * flow methods                 -> apply an action, get the log/reward
//    Deterministic episodes come from DuelConfig's injectable seed.
//
// 2. openjoey::ai::reader  — reads card descriptions and infers effects.
//    Contract it implements (same role as engine/action/Catalog.hpp):
//      * infer(const cards::CardDef&) -> std::vector<ActionSpec>
//    Catalog stays a pure data table (the seed training labels); DuelSetup
//    attaches specs from a source without caring which produced them.
namespace openjoey::ai {
// Intentionally empty — see the contracts above.
}  // namespace openjoey::ai
