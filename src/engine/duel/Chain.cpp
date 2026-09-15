// Chain implementation (duel/Chain.cpp).
#include "engine/duel/Chain.hpp"

namespace openjoey::engine {

void Chain::push(const ActionSpec &spec, int activator, const ActionArgs &args) {
    links.push_back({spec.id, activator, spec.speed, spec, args});
    step = protocol::ChainStep::Building;
    consecutivePasses = 0;  // a new activation reopens the response window
}

void Chain::clear() {
    links.clear();
    step = protocol::ChainStep::Idle;
    consecutivePasses = 0;
}

std::vector<const Chain::Link *> Chain::resolutionOrder() const {
    std::vector<const Link *> order;
    order.reserve(links.size());
    for (auto it = links.rbegin(); it != links.rend(); ++it) order.push_back(&*it);
    return order;
}

bool Chain::legalToChain(uint8_t speed) const {
    if (links.empty()) return true;  // starting a new chain: any Spell Speed may lead
    return speed > 1 && speed >= links.back().speed;
}

}  // namespace openjoey::engine
