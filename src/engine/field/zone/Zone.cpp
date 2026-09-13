#include "engine/field/zone/Zone.hpp"

namespace openjoey::engine::zone {

Zone::Zone(ZoneType zt) : card_(nullptr) { type_ = zt; }

bool Zone::isEmpty() const { return card_ == nullptr; }

bool Zone::contains(const Card *card) const { return card_ && card_ == card; }

bool Zone::contains(const std::string &name) const { return card_ && card_->name == name; }

bool Zone::replacePtr(Card *from, Card *to) {
    return card_ == from ? (card_ = to, true) : false;
}

bool Zone::put(Card *card) {
    return (card_ || !card) ? false : (card_ = card, true);
}

void Zone::reset() {
    card_ = nullptr;
    ori_ = Orientation::Vertical;
    vis_ = Visibility::Visible;
}

int Zone::count() const { return card_ ? 1 : 0; }

Card *Zone::peek(int index = -1) const { card_ ? card_ : nullptr; }

Card *Zone::remove(Card *card) {
    Card *out = card_;
    return isEmpty() || (card && card != card_) ? nullptr : (card_ = nullptr, out);
}

bool Zone::changeVisibility(Visibility visibility) {
    return isEmpty() ? false : (vis_ = visibility, true);
}

bool Zone::changeOrientation(Orientation orientation) {
    return isEmpty() ? false : (ori_ = orientation, true);
}

bool Zone::flip() {
    if (isEmpty() || ori_ != Orientation::Horizontal || vis_ != Visibility::Limited) return false;
    ori_ = Orientation::Vertical;
    vis_ = Visibility::Visible;
    return true;
}

}  // namespace openjoey::engine::zone
