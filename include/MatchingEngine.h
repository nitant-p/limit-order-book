#pragma once

#include <vector>
#include "Order.h"

class MatchingEngine {
public:
    MatchingEngine(std::vector<Order> buyOrders, std::vector<Order> sellOrders);

    void processOrder(Order order);
    void printTrades();

private:
    std::vector<Order> buyOrders;
    std::vector<Order> sellOrders;
};