#pragma once
#include "zone/Field.hpp"
#include <cstdint>

namespace openjoey {

// ─── TargetRequest ────────────────────────────────────────────────────────────
// An Effect::activate() may push one of these into the DuelCore to request a
// player selection. The chain does not advance until fulfilled == true.

struct TargetRequest {
  enum class Kind {
    None,
    OwnMonsterZone,
    OppMonsterZone,
    AnyMonsterZone,
    OwnSpellTrapZone,
    OppSpellTrapZone,
    AnySpellTrapZone,
    OwnHandCard,
    OppHandCard,
    OwnGraveyardCard,
    OppGraveyardCard,
  };

  Kind kind          = Kind::None;
  int  sourcePlayer  = -1; // player whose effect is asking
  int  resolvedPlayer = -1;
  int  resolvedZone   = -1; // zone index on the field
  int  resolvedIndex  = -1; // index into a stack zone
  bool fulfilled      = false;
};

// ─── IDuelContext ─────────────────────────────────────────────────────────────
// Minimal interface that Effects call into. Implemented by DuelCore (Layer 4).
// Defined here (Layer 3) to break the circular include between Effect and
// DuelCore — effects depend on IDuelContext only, not on the full DuelCore.

class IDuelContext {
public:
  virtual ~IDuelContext() = default;

  // Field access — effects read/write zones directly.
  virtual zone::Field &field() = 0;

  // Turn state
  virtual int  turnPlayerIdx() const = 0;
  virtual int  opponentIdx()   const = 0;
  virtual bool hasNormalSummoned(int player) const = 0;
  virtual void setNormalSummoned(int player, bool v) = 0;

  // Win condition
  virtual void setWinner(int player) = 0;

  // Target selection: effect pushes a request; chain waits for fulfillment.
  virtual void pushTargetRequest(TargetRequest req) = 0;
  virtual const TargetRequest &targetRequest() const = 0;
  virtual void clearTargetRequest() = 0;

  // Deterministic RNG (XOR-shift). Seed controlled by DuelCore::setSeed().
  virtual uint32_t nextRng() = 0;

  // Phase query — effects check this to enforce timing restrictions.
  virtual int phase() const = 0; // cast to PhaseManager::Phase

  // ZoneEffectManager hook — persistent effects register themselves here.
  // Called by ContinuousEffect::activate().
  virtual void registerPersistentEffect(class Effect *eff,
                                        zone::IZone   *srcZone,
                                        Card          *srcCard) = 0;

  // Chain control — NegateActivationEffect calls this during its resolve().
  // DuelCore sets an internal flag so the next chain link's resolve() is
  // skipped and its source card is sent to the GY.
  virtual void negateNextChainLink() = 0;
};

} // namespace openjoey
