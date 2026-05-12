#pragma once

#include <vector>
#include <map>
#include <deque>
#include "Order.h"
#include "Trade.h"

class MatchingEngine {
public:
    MatchingEngine(std::map<int, std::deque<Order>, std::greater<int>> buyOrders, std::map<int, std::deque<Order>> sellOrders);

    std::vector<Trade> processOrder(Side side, int price, int quantity);
    void printBook();
    const std::map<int, std::deque<Order>, std::greater<int>>& getBuyOrders() const;
    const std::map<int, std::deque<Order>>& getSellOrders() const;

private:
    uint64_t nextOrderId {1};
    std::map<int, std::deque<Order>, std::greater<int>> buyOrders;
    std::map<int, std::deque<Order>> sellOrders;
    std::vector<Trade> tradeHistory;

    // void removeEmptyOrders(Side side);
    Trade processMatchedOrders(Order& incomingOrder, Order& restingOrder, uint64_t buyId, uint64_t sellId);
    void sortOrders(Side& side, bool isAscending);
    uint64_t getAndIncrementNextOrderId();
    void removeEmptyOrdersFromQueue(std::deque<Order>& orderQueue);
    void removeEmptyOrderQueuesByPrice(std::map<int, std::deque<Order>>& map, int price);

    bool static canOrderPricesMatch(const int buyOrderPrice, const int sellOrderPrice);
};
