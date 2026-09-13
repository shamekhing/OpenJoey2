#pragma once
// ── Zone (openjoey::zone) ────────────────────────────────────────────────────
// A single-card slot: monster zone, spell/trap zone, field zone, EMZ.
// Split out of the old monolithic zone/Zone.hpp — the class name matches the
// header name, mirroring the openjoey-cards convention.

#include "engine/field/zone/IZone.hpp"

namespace openjoey::engine::zone {
using cards::Card;

class Zone : public IZone {
   public:
    explicit Zone(ZoneType zt = ZoneType::None);
    bool isEmpty() const override;
    bool contains(const Card *c) const override;
    bool contains(const std::string &name) const override;
    bool replacePtr(Card *from, Card *to) override;
    bool put(Card *c) override;
    void reset() override;
    int count() const override;
    Card *peek(int index = -1) const override;
    // nullptr removes the occupant; non-null removes only if it matches.
    Card *remove(Card *c = nullptr) override;

    // Face-up/face-down state change (set vs activate). Refuses on an empty
    // zone: callers must never set visibility on nothing, and "true" always
    // means the zone is occupied with the requested visibility.
    bool changeVisibility(Visibility v);
    bool changeOrientation(Orientation p);
    bool flip();

   protected:
    Card *card_ = nullptr;
};

}  // namespace openjoey::engine::zone
