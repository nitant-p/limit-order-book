#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <deque>
#include "Order.h"
#include "Trade.h"

class MatchingEngine {
public:
    MatchingEngine(std::map<int, std::deque<uint64_t>> buyOrders, std::map<int, std::deque<uint64_t>> sellOrders);

    std::vector<Trade> processOrder(Side side, int price, int quantity);
    void printBook();
    const std::map<int, std::deque<uint64_t>>& getBuyOrders() const;
    const std::map<int, std::deque<uint64_t>>& getSellOrders() const;
    bool cancelOrder(uint64_t cancelId);

private:
    uint64_t nextOrderId {1};
    std::unordered_map<uint64_t, Order> idToOrderMap;
    std::map<int, std::deque<uint64_t>> buyOrders;
    std::map<int, std::deque<uint64_t>> sellOrders;
    std::vector<Trade> tradeHistory;
    

    // void removeEmptyOrders(Side side);
    Trade processMatchedOrders(Order& incomingOrder, Order& restingOrder, uint64_t buyId, uint64_t sellId);
    void sortOrders(Side& side, bool isAscending);
    uint64_t getAndIncrementNextOrderId();
    void deleteEmptyOrderInOrder(uint64_t orderId, std::deque<uint64_t>& orderQueue);
    void removeEmptyOrderQueuesByPrice(std::map<int, std::deque<uint64_t>>& map, int price);
    void addNewOrder(Side side, Order& newOrder);
    bool static canOrderPricesMatch(const int buyOrderPrice, const int sellOrderPrice);

};
