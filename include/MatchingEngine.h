#pragma once

#include <vector>
#include "Order.h"

class MatchingEngine {
public:
    MatchingEngine(std::vector<Order> buyOrders, std::vector<Order> sellOrders);

    void processOrder(Order newOrder);
    void printBook();
    const std::vector<Order>& getBuyOrders() const;
    const std::vector<Order>& getSellOrders() const;

private:
    std::vector<Order> buyOrders;
    std::vector<Order> sellOrders;

    void removeEmptyOrders(Side side);
    void processMatchedOrders(Order& order1, Order& order2);
};
