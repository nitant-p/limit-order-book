#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "Order.h"

struct LevelSnapshot {
    int price;
    uint64_t totalQuantity;
    size_t orderCount;
};

struct OrderNode {
    Order order;
    OrderNode* prev = nullptr;
    OrderNode* next = nullptr;
};

struct PriceLevel {
    OrderNode* head = nullptr;
    OrderNode* end = nullptr;

    size_t orderCount = 0;
    uint64_t totalQuantity = 0;
};

class OrderBookSide {
public:
    explicit OrderBookSide(Side side);

    // Basic state
    Side side() const;
    bool empty() const;
    size_t priceLevelCount() const;
    size_t orderCount() const;

    // Best price on this side
    std::optional<int> bestPrice() const;

    // Order lookup
    bool doesOrderExist(uint64_t orderId) const;
    const Order* findOrder(uint64_t orderId) const;
    Order* findOrder(uint64_t orderId);

    // Order operations
    void addOrder(uint64_t orderId, Side side, Type type, int price, int quantity);
    void deleteOrderById(uint64_t orderId);

    // Price level lookup
    bool doesLevelExist(int price) const;
    const PriceLevel* findLevel(int price) const;
    PriceLevel* findLevel(int price);

    // Price level cleanup
    void removeLevelIfEmpty(int price);

    // Price level stats
    size_t orderCountAtPrice(int price) const;
    uint64_t volumeAtPrice(int price) const;

    // Book views
    std::vector<LevelSnapshot> getDepth(size_t levels) const;
    std::vector<uint64_t> getAllOrderIds() const;
    std::vector<uint64_t> getOrderIdsAtPrice(int price) const;

private:
    Side side_;

    // Price -> linked list of orders at that price
    std::map<int, PriceLevel> priceToLevels_;

    // Order ID -> owning pointer to actual order node
    std::map<uint64_t, std::unique_ptr<OrderNode>> orderNodesById_;
};