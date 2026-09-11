#include "cards/Card.hpp"
using namespace openjoey::cards;

bool CardDef::hasAttribute(Attribute a) const {
    return std::find(attributes.begin(), attributes.end(), a) != attributes.end();
}

bool CardDef::hasAttributes(const std::vector<Attribute> &as, bool any) const {
    for (const auto &a : as)
        if (hasAttribute(a) == any)
            return true;
    return !any;
}

bool CardDef::isMonster() const { return hasAttribute(Attribute::Monster); }
bool CardDef::isSpell()  const { return hasAttribute(Attribute::Spell); }
bool CardDef::isTrap()   const { return hasAttribute(Attribute::Trap); }

bool CardDef::isExtraDeckMonster() const {
    return hasAttributes({Attribute::Fusion, Attribute::Synchro, Attribute::Xyz}, true);
}

bool Card::operator==(const Card &other) const { return id != 0 && id == other.id; }
bool Card::operator!=(const Card &other) const { return !(*this == other); }

int Card::effectiveAtk() const { return std::max(0, atk + state.atkMod); }
int Card::effectiveDef() const { return std::max(0, def + state.defMod); }

std::string Card::cardTypeTag() const {
    return isMonster() ? "[MON]" : isSpell() ? "[SPL]" : isTrap() ? "[TRP]" : "[UNK]";
}

std::string Card::statLine() const {
    return isMonster()
        ? "Level " + std::to_string(level) + "  ATK " + std::to_string(atk) + "  DEF " + std::to_string(def)
        : "";
}

std::string Card::shortStat() const {
    return isMonster()
        ? "L" + std::to_string(level) + " " + std::to_string(atk) + "/" + std::to_string(def)
        : "";
}