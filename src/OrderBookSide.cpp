//
// Created by Nitant Panicker on 17/5/26.
//

#include "../include/OrderBookSide.h"

#include <ranges>

using namespace std;

OrderBookSide::OrderBookSide(Side side):side_(side) {}

Side OrderBookSide::side() const {
    return this->side_;
}

bool OrderBookSide::empty() const {
    return this->levels_.empty();
}

size_t OrderBookSide::priceLevelCount() const {
    return this->levels_.size();
}

std::optional<int> OrderBookSide::bestPrice() const {
    if (this->levels_.empty()) return nullopt;

    if (this->side_ == Side::BUY) {
        auto it = this->levels_.end();
        --it;
        return it->first;
    } else {
        return this->levels_.begin()->first;
    }
}

void OrderBookSide::addOrder(int price, uint64_t orderId) {
    this->levels_[price].push_back(orderId);
}

const deque<uint64_t>* OrderBookSide::findLevel(int price) const {
    auto it = levels_.find(price);
    if (it == levels_.end()) return nullptr;
    return &it->second;
}

deque<uint64_t>* OrderBookSide::findLevel(int price) {
    auto it = levels_.find(price);
    if (it == levels_.end()) return nullptr;
    return &it->second;
}

void OrderBookSide::removeLevelIfEmpty(int price) {
    auto it = levels_.find(price);
    // if does not exist or not empty. do nothing
    if (it == levels_.end() or !it->second.empty()) return;

    levels_.erase(it);
}

size_t OrderBookSide::orderCountAtPrice(int price) const {
    auto it = levels_.find(price);
    if (it == levels_.end()) return 0;
    else return it->second.size();
}

uint64_t OrderBookSide::volumeAtPrice(int price, const std::unordered_map<uint64_t, Order>& ordersById) const {
    const deque<uint64_t>* ordersAtLevel = findLevel(price);
    if (ordersAtLevel == nullptr) return 0;

    uint64_t quantitySum = 0;
    for (auto it = ordersAtLevel->begin(); it != ordersAtLevel->end(); ++it) {
        auto o = ordersById.find(*it);
        if (o != ordersById.end()) quantitySum += static_cast<uint64_t>(o->second.quantity);
    }
    return quantitySum;
}

vector<LevelSnapshot> OrderBookSide::getDepth(size_t levels, const unordered_map<uint64_t, Order>& ordersById) const {
    vector<LevelSnapshot> levelSnapshots;
    if (side_ == Side::BUY) {
        for (const auto& l : this->levels_ | ranges::views::reverse) {
            uint64_t totalQuantity = 0;
            const size_t orderCount = l.second.size();
            for (auto it = l.second.begin(); it != l.second.end(); ++it) {
                const uint64_t& orderId = *it;
                auto orderEntry = ordersById.find(orderId);
                if (orderEntry == ordersById.end()) continue;
                const Order& order = orderEntry->second;
                totalQuantity += order.quantity;
            }
            levelSnapshots.push_back({l.first, totalQuantity, orderCount});
            if (levelSnapshots.size() == levels) break;
        }
    } else {
        for (auto it = this->levels_.begin(); it != this->levels_.end(); ++it) {
            const deque<uint64_t>& orderQueue = it->second;
            uint64_t totalQuantity = 0;
            const size_t orderCount = orderQueue.size();
            for (auto qIt = orderQueue.begin(); qIt != orderQueue.end(); ++qIt) {
                const uint64_t& orderId = *qIt;
                auto orderEntry = ordersById.find(orderId);
                if (orderEntry == ordersById.end()) continue;
                const Order& order = orderEntry->second;
                totalQuantity += order.quantity;
            }
            levelSnapshots.push_back({it->first, totalQuantity, orderCount});
            if (levelSnapshots.size() == levels) break;
        }
    }

    return levelSnapshots;
}

vector<uint64_t> OrderBookSide::getAllOrderIds() {
    vector<uint64_t> orderIds;
    for (auto it = levels_.begin(); it != levels_.end(); ++it) {
        deque<uint64_t> queue = it->second;
        for (auto qIt = queue.begin(); qIt != queue.end(); ++qIt) orderIds.push_back(*qIt);
    }
    return orderIds;
}