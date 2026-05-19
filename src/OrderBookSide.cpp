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
    return this->priceToLevels_.empty();
}

size_t OrderBookSide::priceLevelCount() const {
    return this->priceToLevels_.size();
}

std::optional<int> OrderBookSide::bestPrice() const {
    if (this->priceToLevels_.empty()) return nullopt;

    if (this->side_ == Side::BUY) {
        auto it = this->priceToLevels_.end();
        --it;
        return it->first;
    } else {
        return this->priceToLevels_.begin()->first;
    }
}

void OrderBookSide::addOrder(uint64_t orderId, Side side, Type type, int price, int quantity) {
    if (orderNodesById_.contains(orderId) || side != this->side_) return;

    Order order(orderId, side, type, price, quantity);
    auto priceToLevelPtr = priceToLevels_.find(price);
    auto newNode = std::make_unique<OrderNode>(OrderNode{order, nullptr, nullptr});
    
    OrderNode* nodePtr = newNode.get();
    orderNodesById_[orderId] = move(newNode);

    if (priceToLevelPtr == priceToLevels_.end()) {
        // make new level
        priceToLevels_[price] = PriceLevel{nodePtr, nodePtr, 1, static_cast<uint64_t>(nodePtr->order.quantity)};
    } else {
        // join at end, update end
        auto end = priceToLevelPtr->second.end;
        end->next = nodePtr;
        nodePtr->prev = end;
        priceToLevelPtr->second.end = nodePtr;
        ++priceToLevelPtr->second.orderCount;
        priceToLevelPtr->second.totalQuantity += nodePtr->order.quantity;
    }
}

bool OrderBookSide::doesOrderExist(uint64_t orderId) const {
    auto it = orderNodesById_.find(orderId);
    return it != orderNodesById_.end(); 
}

void OrderBookSide::deleteOrderById(uint64_t id) {
    if (!doesOrderExist(id)) return;

    auto nodePtr = orderNodesById_[id].get();
    // get prev and next
    auto prev = nodePtr->prev;
    auto next = nodePtr->next;

    // unlink
    if (prev != nullptr) prev->next = next;
    if (next != nullptr) next->prev = prev;

    PriceLevel& priceLevel = priceToLevels_[nodePtr->order.price];
    if (priceLevel.head == nodePtr) priceLevel.head = nodePtr->next;
    if (priceLevel.end == nodePtr) priceLevel.end = nodePtr->prev;

    --priceLevel.orderCount;
    priceLevel.totalQuantity -= nodePtr->order.quantity;

    // delete empty price level
    if (priceLevel.head == nullptr && priceLevel.end == nullptr) priceToLevels_.erase(nodePtr->order.price);

    // finally delete order
    orderNodesById_.erase(id);
}

const PriceLevel* OrderBookSide::findLevel(int price) const {
    auto it = priceToLevels_.find(price);
    if (it == priceToLevels_.end()) return nullptr;
    return &it->second;
}

PriceLevel* OrderBookSide::findLevel(int price) {
    auto it = priceToLevels_.find(price);
    if (it == priceToLevels_.end()) return nullptr;
    return &it->second;
}

void OrderBookSide::removeLevelIfEmpty(int price) {
    auto it = priceToLevels_.find(price);
    // if does not exist or not empty, do nothing
    if (it == priceToLevels_.end() or it->second.head != nullptr) return;

    priceToLevels_.erase(it);
}

size_t OrderBookSide::orderCountAtPrice(int price) const {
    auto it = priceToLevels_.find(price);
    if (it == priceToLevels_.end()) return 0;
    else return it->second.orderCount;
}

uint64_t OrderBookSide::volumeAtPrice(int price) const {
    const PriceLevel* ptr = findLevel(price);
    if (ptr == nullptr) return 0;
    return ptr->totalQuantity;
}

vector<LevelSnapshot> OrderBookSide::getDepth(size_t levels) const {
    vector<LevelSnapshot> levelSnapshots;
    if (side_ == Side::BUY) {
        for (const auto& l : this->priceToLevels_ | ranges::views::reverse) {
            levelSnapshots.push_back({l.first, l.second.totalQuantity, l.second.orderCount});
            if (levelSnapshots.size() == levels) break;
        }
    } else {
        for (auto it = this->priceToLevels_.begin(); it != this->priceToLevels_.end(); ++it) {
            levelSnapshots.push_back({it->first, it->second.totalQuantity, it->second.orderCount});
            if (levelSnapshots.size() == levels) break;
        }
    }

    return levelSnapshots;
}

vector<uint64_t> OrderBookSide::getAllOrderIds() const {
    vector<uint64_t> orderIds;
    for (const auto& [orderId, node] : orderNodesById_) {
        orderIds.push_back(orderId);
    }
    return orderIds;
}

size_t OrderBookSide::orderCount() const {
    return orderNodesById_.size();
}

const Order* OrderBookSide::getBestOrder() const {
    auto best = bestPrice();
    if (!best) return nullptr;

    const PriceLevel* level = findLevel(*best);
    if (level == nullptr or level->head == nullptr) return nullptr;

    return &level->head->order;
}

void OrderBookSide::removeOrderIfFilled(uint64_t id) {
    auto it = orderNodesById_.find(id);
    if (it == orderNodesById_.end()) return;

    // check if order is empty and needs to be removed
    auto nodePtr = it->second.get();
    if (nodePtr->order.quantity > 0) return;

    deleteOrderById(nodePtr->order.id);
}

bool OrderBookSide::reduceOrderQuantity(uint64_t id, int delta) {
    auto it = orderNodesById_.find(id);
    if (it == orderNodesById_.end()) return false;

    Order& order = it->second.get()->order;

    if (delta > order.quantity) return false;
    order.quantity -= delta;

    if (order.quantity == 0) {
        deleteOrderById(order.id);
        return true;
    }

    // reduce price level data
    PriceLevel& level = *findLevel(order.price);
    level.totalQuantity -= delta;
}
