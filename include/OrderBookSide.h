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
    void addOrder(Order& order);

    void deleteOrderById(uint64_t orderId);

    // Price level lookup
    bool doesLevelExist(int price) const;
    const PriceLevel* findLevel(int price) const;
    PriceLevel* findLevel(int price);
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

    const Order* getBestOrder() const;
    void removeOrderIfFilled(uint64_t id);
    bool reduceOrderQuantity(uint64_t id, int quantity);
    bool modifyOrder(Order updatedOrder, bool loseQueuePos);

private:
    Side side_;

    // Price -> linked list of orders at that price
    std::map<int, PriceLevel> priceToLevels_;

    // Order ID -> owning pointer to actual order node
    std::map<uint64_t, std::unique_ptr<OrderNode>> orderNodesById_;
};


1. Find the OrderNode by order ID.
2. Save original price and quantity.
3. Find original PriceLevel.
4. Determine new price and new quantity.
5. Validate quantity/price if needed.
6. If price does not change and queue position is not lost:
   - update quantity
   - update level totalQuantity by newQuantity - originalQuantity
   - return true
7. Otherwise:
   - unlink node from original level
   - update original level head/end/orderCount/totalQuantity
   - remove original level if empty
   - update node's order fields
   - create/find new level
   - append node to end of new level
   - update new level head/end/orderCount/totalQuantity
   - return true