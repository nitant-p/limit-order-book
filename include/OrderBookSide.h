#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

#include "Order.h"

struct LevelSnapshot {
    int price;
    uint64_t totalQuantity;
    size_t orderCount;
};

struct OrderNode {
    Order order;
    OrderNode* prev;
    OrderNode* next;
};

struct PriceLevel {
    OrderNode* head;
    OrderNode* end;
};

class OrderBookSide {
public:
    explicit OrderBookSide(Side side);

    Side side() const;
    bool empty() const;
    size_t priceLevelCount() const;
    std::optional<int> bestPrice() const;

    void addOrder(uint64_t orderId, Side side, Type type, int price, int quantity);

    const std::deque<uint64_t>* findLevel(int price) const;
    std::deque<uint64_t>* findLevel(int price);

    void removeLevelIfEmpty(int price);

    size_t orderCountAtPrice(int price) const;
    uint64_t volumeAtPrice(int price, const std::unordered_map<uint64_t, Order>& ordersById) const;

    std::vector<LevelSnapshot> getDepth(
        size_t levels,
        const std::unordered_map<uint64_t, Order>& ordersById
    ) const;

    std::vector<uint64_t> getAllOrderIds();
private:
    Side side_;
    std::map<int, PriceLevel> priceToLevels_;
    std::map<uint64_t, OrderNode*> idToOrderNodePtr;
};
