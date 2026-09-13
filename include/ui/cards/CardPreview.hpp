#pragma once
#include <raylib.h>

#include <string>
#include <vector>

#include "cards/Card.hpp"
#include "ui/cards/CardImageCache.hpp"

namespace openjoey::ui {

class CardPreview {
   public:
    void SetCard(const openjoey::cards::Card* card, bool faceDown = false);
    void SetCardBack(const Texture2D* cb);
    void scroll(int delta);
    void Draw(Rectangle bounds, CardImageCache& cache) const;

   private:
    const openjoey::cards::Card* card_ = nullptr;
    bool faceDown_ = false;
    const Texture2D* cardBack_ = nullptr;
    int scrollLines_ = 0;
    mutable const openjoey::cards::Card* wrappedFor_ = nullptr;
    mutable int wrappedWidth_ = -1;
    mutable std::vector<std::string> wrappedLines_;
};

}  // namespace openjoey::ui
