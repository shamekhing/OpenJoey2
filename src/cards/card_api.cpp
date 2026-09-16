// Card API implementation (cards/card_api.cpp).
#include "cards/card_api.hpp"

namespace openjoey::cards {

bool LoadDatabase(CardDatabase &db, const std::string &path) {
    return db.LoadFromFile(path);
}

}  // namespace openjoey::cards
