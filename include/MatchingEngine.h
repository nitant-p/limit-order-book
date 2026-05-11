#pragma once

#include <vector>
#include "Order.h"
#include "Trade.h"

class MatchingEngine {
public:
    MatchingEngine(std::vector<Order> buyOrders, std::vector<Order> sellOrders);

    std::vector<Trade> processOrder(Side side, int price, int quantity);
    void printBook();
    const std::vector<Order>& getBuyOrders() const;
    const std::vector<Order>& getSellOrders() const;

private:
    uint64_t nextOrderId {1};
    std::vector<Order> buyOrders;
    std::vector<Order> sellOrders;
    std::vector<Trade> tradeHistory;

    void removeEmptyOrders(Side side);
    Trade processMatchedOrders(Order& incomingOrder, Order& restingOrder, uint64_t buyId, uint64_t sellId);
    void sortOrders(Side& side, bool isAscending);
    uint64_t getAndIncrementNextOrderId();
};
