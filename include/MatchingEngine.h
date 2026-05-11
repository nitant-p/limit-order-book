#pragma once

#include <vector>
#include "Order.h"
#include "Trade.h"

class MatchingEngine {
public:
    MatchingEngine(std::vector<Order> buyOrders, std::vector<Order> sellOrders);

    std::vector<Trade> processOrder(Order newOrder);
    void printBook();
    const std::vector<Order>& getBuyOrders() const;
    const std::vector<Order>& getSellOrders() const;

private:
    std::vector<Order> buyOrders;
    std::vector<Order> sellOrders;
    std::vector<Trade> tradeHistory;

    void removeEmptyOrders(Side side);
    Trade processMatchedOrders(Order& incomingOrder, Order& restingOrder, int buyId, int sellId);
    void sortOrders(Side& side, bool isAscending);
};
