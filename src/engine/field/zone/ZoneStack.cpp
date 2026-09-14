#include "engine/field/zone/ZoneStack.hpp"

namespace openjoey::engine::zone {

ZoneStack::ZoneStack(ZoneType zt) { type_ = zt; }

Card* ZoneStack::operator[] (size_t i) const {
    return cards_[i];
}

bool ZoneStack::isEmpty() const { return cards_.empty(); }

void ZoneStack::reset() { cards_.clear(); }

int ZoneStack::count() const { return static_cast<int>(cards_.size()); }

bool ZoneStack::put(Card *card) {
    return !card ? false : (cards_.push_back(card), true);
}

bool ZoneStack::contains(const Card *card) const {
    return std::find(cards_.begin(), cards_.end(), card) != cards_.end();
}

bool ZoneStack::contains(const std::string &name) const {
    for (const Card *card : cards_)
        if (card->name == name) return true;
    return false;
}

Card *ZoneStack::peek(int index) const {
    if (cards_.empty()) return nullptr;
    if (index < 0) return cards_.back();
    return index < static_cast<int>(cards_.size()) ? cards_[index] : nullptr;
}

Card *ZoneStack::remove(Card *card) {
    if (cards_.empty()) return nullptr;
    if (!card) {
        card = cards_.back();
        cards_.pop_back();
        return card;
    }
    auto it = std::find(cards_.begin(), cards_.end(), card);
    if (it == cards_.end()) return nullptr;
    cards_.erase(it);
    return card;
}

std::vector<Card *> ZoneStack::findAll(std::function<bool(const Card *)> pred) const {
    std::vector<Card *> result;
    for (Card *card : cards_)
        if (pred(card)) result.push_back(card);
    return result;
}

void ZoneStack::shuffle() {
    static std::mt19937 rng{std::random_device{}()};
    std::shuffle(cards_.begin(), cards_.end(), rng);
}

bool ZoneStack::replacePtr(Card *from, Card *to) {
    bool hit = false;
    for (Card *&card : cards_)
        if (card == from) {
            card = to;
            hit = true;
        }
    return hit;
}

const std::vector<Card *> &ZoneStack::cards() const { return cards_; }

}  // namespace openjoey::engine::zone
