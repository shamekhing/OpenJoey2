#include "cards/CardEnums.hpp"

#include <cctype>
#include <string>
#include <unordered_map>

namespace openjoey::cards {

std::string normaliseString(const std::string &str) {
    std::string out;
    out.reserve(str.size());
    for (unsigned char ch : str)
        if (std::isalnum(ch))
            out.push_back(static_cast<char>(std::tolower(ch)));
    return out;
}

Attribute attribute_from_string(const std::string &str) {
    static const std::unordered_map<std::string, Attribute> str2attribute = {
        {"none", Attribute::None},
        {"monster", Attribute::Monster}, {"spell", Attribute::Spell}, {"trap", Attribute::Trap},
        {"skill", Attribute::Skill}, {"token", Attribute::Token},
        {"dark", Attribute::Dark}, {"light", Attribute::Light}, {"earth", Attribute::Earth},
        {"water", Attribute::Water}, {"fire", Attribute::Fire}, {"wind", Attribute::Wind},
        {"divine", Attribute::Divine},
        {"normal", Attribute::Normal}, {"effect", Attribute::Effect}, {"fusion", Attribute::Fusion},
        {"ritual", Attribute::Ritual}, {"synchro", Attribute::Synchro}, {"xyz", Attribute::Xyz},
        {"link", Attribute::Link}, {"flip", Attribute::Flip}, {"tuner", Attribute::Tuner},
        {"spirit", Attribute::Spirit}, {"gemini", Attribute::Gemini}, {"union", Attribute::Union},
        {"toon", Attribute::Toon}, {"pendulum", Attribute::Pendulum}, {"illusion", Attribute::Illusion},
        {"equip", Attribute::Equip}, {"continuous", Attribute::Continuous}, {"field", Attribute::Field},
        {"quickplay", Attribute::QuickPlay}, {"counter", Attribute::Counter},
        {"limited", Attribute::Limited}, {"forbidden", Attribute::Forbidden},
        {"semilimited", Attribute::SemiLimited}, {"unlimited", Attribute::Unlimited},
        {"left", Attribute::LinkMarkerLeft}, {"right", Attribute::LinkMarkerRight},
        {"top", Attribute::LinkMarkerTop}, {"bottom", Attribute::LinkMarkerBottom},
        {"topleft", Attribute::LinkMarkerTopLeft}, {"topright", Attribute::LinkMarkerTopRight},
        {"bottomleft", Attribute::LinkMarkerBottomLeft}, {"bottomright", Attribute::LinkMarkerBottomRight},
        {"dragon", Attribute::Dragon}, {"spellcaster", Attribute::Spellcaster},
        {"warrior", Attribute::Warrior}, {"fairy", Attribute::Fairy}, {"fiend", Attribute::Fiend},
        {"zombie", Attribute::Zombie}, {"machine", Attribute::Machine}, {"aqua", Attribute::Aqua},
        {"pyro", Attribute::Pyro}, {"rock", Attribute::Rock}, {"wingedbeast", Attribute::WingedBeast},
        {"plant", Attribute::Plant}, {"insect", Attribute::Insect}, {"thunder", Attribute::Thunder},
        {"beast", Attribute::Beast}, {"beastwarrior", Attribute::BeastWarrior},
        {"dinosaur", Attribute::Dinosaur}, {"fish", Attribute::Fish},
        {"seaserpent", Attribute::SeaSerpent}, {"reptile", Attribute::Reptile},
        {"psychic", Attribute::Psychic}, {"divinebeast", Attribute::DivineBeast},
        {"creatorgod", Attribute::CreatorGod}, {"wyrm", Attribute::Wyrm}, {"cyberse", Attribute::Cyberse},
    };
    auto it = str2attribute.find(normaliseString(str));
    return it != str2attribute.end() ? it->second : Attribute::None;
}

} // namespace openjoey::cards
