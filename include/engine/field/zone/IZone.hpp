#pragma once
// ── IZone (openjoey::zone) ───────────────────────────────────────────────────
// Abstract base for all zone types. A zone owns no cards — it holds raw
// pointers. Ownership is managed by the game engine.
// Split out of the old monolithic zone/Zone.hpp; enums live in ZoneEnums.hpp.

#include "cards/Card.hpp"
#include "engine/field/zone/ZoneEnums.hpp"

namespace openjoey::engine::zone {
using cards::Card;

class IZone {
   public:
    virtual ~IZone() = default;

    virtual Card* operator[] (size_t i) const = 0;

    virtual ZoneType type() const { return type_; }
    virtual bool isEmpty()  const = 0;
    virtual int count()     const = 0;

    virtual bool put(Card *c) = 0;
    virtual bool contains(const Card *c) const = 0;
    virtual bool contains(const std::string &name) const = 0;
    virtual void reset() {}

    // nullptr → remove top/only occupant.
    virtual Card *remove(Card *c = nullptr)  = 0;
    virtual Card *peek(int index = -1) const = 0;
    // Move the top/only card to dest. Rolls back if dest.put fails.
    bool moveTo(IZone &dest);

    void type(ZoneType zt) { type_ = zt; }

    // Swap a held pointer (snapshot/undo token remap). Default: unsupported.
    virtual bool replacePtr(Card *from, Card *to) {
        (void)from;
        (void)to;
        return false;
    }

    Visibility visibility() const { return vis_; }
    Orientation orientation() const { return ori_; }
    
    bool isVertical() const { return ori_ == Orientation::Vertical; }
    bool isHorizontal() const { return ori_ == Orientation::Horizontal; }
    bool isVisible() const { return vis_ == Visibility::Visible; }
    bool isLimited() const { return vis_ == Visibility::Limited; }
    bool isRestricted() const { return vis_ == Visibility::Restricted; }

   protected:
    Orientation ori_ = Orientation::Vertical;
    Visibility vis_ = Visibility::Visible;
    ZoneType type_ = ZoneType::None;
    bool enabled_ = true;
};

}  // namespace openjoey::engine::zone