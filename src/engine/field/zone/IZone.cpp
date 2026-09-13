#include "engine/field/zone/IZone.hpp"

namespace openjoey::engine::zone {

bool IZone::moveTo(IZone &dest) {
    Card *card = remove(nullptr);
    return !card ? false : (!dest.put(card) ? (put(card), false) : true);
}

}  // namespace openjoey::engine::zone
