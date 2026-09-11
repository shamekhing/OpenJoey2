#include "cards/CardDatabase.hpp"
using namespace openjoey::cards;

void CardDatabase::Clear() {
    cards_.clear(); byId_.clear(); byName_.clear(); byAttribute_.clear();
}

std::size_t CardDatabase::size() const { return cards_.size(); }
bool CardDatabase::empty() const { return cards_.empty(); }

Card *CardDatabase::GetCardById(uint32_t id) {
    auto it = byId_.find(id);
    return it != byId_.end() ? it->second : nullptr;
}
const Card *CardDatabase::GetCardById(uint32_t id) const {
    auto it = byId_.find(id);
    return it != byId_.end() ? it->second : nullptr;
}

Card *CardDatabase::GetCardByName(const std::string &name) {
    auto it = byName_.find(name);
    return it != byName_.end() ? it->second : nullptr;
}
const Card *CardDatabase::GetCardByName(const std::string &name) const {
    auto it = byName_.find(name);
    return it != byName_.end() ? it->second : nullptr;
}

std::vector<const Card *> CardDatabase::FindByName(const std::string &name) const {
    std::vector<const Card *> out;
    for (const Card &c : cards_)
        if (c.name.find(name) != std::string::npos) out.push_back(&c);
    std::sort(out.begin(), out.end(), [](const Card *a, const Card *b) { return a->id < b->id; });
    return out;
}

std::vector<const Card *> CardDatabase::GetCardByAttribute(Attribute attr) const {
    std::vector<const Card *> out;
    for (const Card &c : cards_)
        if (c.hasAttribute(attr)) out.push_back(&c);
    std::sort(out.begin(), out.end(), [](const Card *a, const Card *b) { return a->id < b->id; });
    return out;
}

const std::vector<Card> &CardDatabase::GetAllCards() const { return cards_; }

bool CardDatabase::LoadFromString(const std::string &content) {
    Clear();
    ParseResult parsed = parseRemoteCardJson(content);

    if (!parsed.ok())
        return false;

    cards_ = std::move(parsed.cards);
    byId_.reserve(cards_.size());
    byName_.reserve(cards_.size());
    byAttribute_.reserve(cards_.size());

    for (Card &c : cards_) {
        byId_[c.id] = &c;
        if (byName_.find(c.name) == byName_.end())
            byName_[c.name] = &c;
        for (Attribute a : c.attributes)
            byAttribute_[a].push_back(&c);
    }
    return true;
}

bool CardDatabase::LoadFromFile(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return LoadFromString(content);
}