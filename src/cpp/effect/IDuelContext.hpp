#pragma once
#include "field/Field.hpp"
#include <cstdint>

namespace openjoey {

/**
 * Pending player choice requested by an effect.
 *
 * The chain pauses while fulfilled == false; UI/adapters can fill in the
 * resolved fields before DuelCore continues resolution.
 */
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

/**
 * Minimal interface that effects call into.
 *
 * Effects depend on this abstraction instead of DuelCore directly, which keeps
 * effect headers independent from the full duel implementation.
 */
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
  virtual int phase() const = 0; // cast from etypes::phase

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
