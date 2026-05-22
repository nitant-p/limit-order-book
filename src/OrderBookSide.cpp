//
// Created by Nitant Panicker on 17/5/26.
//

#include "../include/OrderBookSide.h"
#include <iostream>
#include <ranges>
#include <utility>

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
    orderNodesById_[orderId] = std::move(newNode);

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

void OrderBookSide::addOrder(Order& order) {
    addOrder(order.id, order.side, order.type, order.price, order.quantity);
}

bool OrderBookSide::doesOrderExist(uint64_t orderId) const {
    auto it = orderNodesById_.find(orderId);
    return it != orderNodesById_.end(); 
}

bool OrderBookSide::deleteOrderById(uint64_t id) {
    if (!doesOrderExist(id)) return false;

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
    removeLevelIfEmpty(nodePtr->order.price);

    // finally delete order
    orderNodesById_.erase(id);

    return true;
}

bool OrderBookSide::doesLevelExist(int price) const {
    return priceToLevels_.contains(price);
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
    if (levels == 0) return levelSnapshots;

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

vector<uint64_t> OrderBookSide::getOrderIdsAtPrice(int price) const {
    vector<uint64_t> ids;
    const PriceLevel* level = findLevel(price);
    if (level == nullptr) return ids;

    for (OrderNode* curr = level->head; curr != nullptr; curr = curr->next) {
        ids.push_back(curr->order.id);
    }

    return ids;
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

    // reduce price level data
    PriceLevel& level = *findLevel(order.price);
    level.totalQuantity -= delta;

    order.quantity -= delta;
    if (order.quantity == 0) {
        deleteOrderById(order.id);
        return true;
    }
    
    return true;
}

Order* OrderBookSide::findOrderMutable(uint64_t orderId) {
    auto it = orderNodesById_.find(orderId);
    if (it == orderNodesById_.end()) return nullptr;
    return &it->second.get()->order;
}


const Order* OrderBookSide::findOrder(uint64_t orderId) const {
    auto it = orderNodesById_.find(orderId);
    if (it == orderNodesById_.end()) return nullptr;
    return &it->second.get()->order;
}


bool OrderBookSide::modifyOrder(Order updatedOrder, bool loseQueuePos) {
    if (updatedOrder.quantity <= 0) return false;

    Order* orderPtr = findOrderMutable(updatedOrder.id);
    if (orderPtr == nullptr) return false;

    PriceLevel* originalLevel = findLevel(orderPtr->price);
    if (originalLevel == nullptr) return false;

    int originalQuantity = orderPtr->quantity;
    int originalPrice = orderPtr->price;

    if (!loseQueuePos and originalPrice == updatedOrder.price) {
        // only update order and level quantity
        orderPtr->quantity = updatedOrder.quantity;
        originalLevel->totalQuantity -= originalQuantity;
        originalLevel->totalQuantity += updatedOrder.quantity;
        return true;
    }    

    auto ptr = orderNodesById_.find(orderPtr->id);
    if (ptr == orderNodesById_.end()) return false;

    OrderNode* node = ptr->second.get();
    OrderNode* prev = node->prev;
    OrderNode* next = node->next;

    if (prev != nullptr) prev->next = next;
    if (next != nullptr) next->prev = prev;

    // unlink
    node->next = nullptr;
    node->prev = nullptr;

    if (originalLevel->head == node) originalLevel->head = next;
    if (originalLevel->end == node) originalLevel->end = prev;

    if (originalPrice == updatedOrder.price) {
        orderPtr->quantity = updatedOrder.quantity;
        // move to back of queue
        // join node at the end
        node->prev = originalLevel->end;
        if (originalLevel->end != nullptr) originalLevel->end->next = node;

        // update level end
        originalLevel->end = node;
        // check level head is not null
        if (originalLevel->head == nullptr) originalLevel->head = node;

        // update quantity
        originalLevel->totalQuantity -= originalQuantity;
        originalLevel->totalQuantity += updatedOrder.quantity;
        return true;
    }

    // totally remove and add to new level
    originalLevel->totalQuantity -= originalQuantity;
    --originalLevel->orderCount;
    removeLevelIfEmpty(originalPrice);

    // update price and quantity
    orderPtr->price = updatedOrder.price;
    orderPtr->quantity = updatedOrder.quantity;

    PriceLevel* newLevel = findLevel(orderPtr->price);

    if (newLevel == nullptr) {
        // create if missing
        PriceLevel p{node, node, 1, static_cast<uint64_t>(orderPtr->quantity)};
        priceToLevels_[orderPtr->price] = p;
        return true;
    }

    if (newLevel->orderCount == 0) {
        node->prev = nullptr;
        node->next = nullptr;

        newLevel->head = node;
        newLevel->end = node;
        newLevel->orderCount = 1;
        newLevel->totalQuantity = updatedOrder.quantity;

        return true;
    }

    OrderNode* levelEnd = newLevel->end;
    levelEnd->next = node;
    node->prev = levelEnd;
    ++newLevel->orderCount;
    newLevel->totalQuantity += orderPtr->quantity;
    newLevel->end = node;

    return true;
}

void OrderBookSide::printBook() {
    for (auto it = orderNodesById_.begin(); it != orderNodesById_.end(); ++it) {
        cout << it->second.get()->order << endl;
    }
}
