#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <deque>
#include "Order.h"
#include "OrderBookSide.h"
#include "Trade.h"

class MatchingEngine {
public:
    MatchingEngine(OrderBookSide buyBook, OrderBookSide sellBook);

    std::vector<Trade> processOrder(Side side, Type type, int price, int quantity);
    void printBook();
    const OrderBookSide& getBuyBook() const;
    const OrderBookSide& getSellOrders() const;
    bool cancelOrder(uint64_t cancelId);
    bool modifyOrder(uint64_t orderId, int newPrice, int newQuantity);

private:
    uint64_t nextOrderId {1};

    OrderBookSide buyBook;
    OrderBookSide sellBook;
    std::vector<Trade> tradeHistory;
    

    // void removeEmptyOrders(Side side);
    Trade processMatchedOrders(Order& incomingOrder, const Order& restingOrder, uint64_t buyId, uint64_t sellId, OrderBookSide& book);
    uint64_t getAndIncrementNextOrderId();
    void deleteEmptyOrderInOrder(uint64_t orderId, std::deque<uint64_t>& orderQueue);
    void removeEmptyOrderQueuesByPrice(std::map<int, std::deque<uint64_t>>& map, int price);
    void addNewOrder(Side side, Order& newOrder);
    bool static canContinueAgainstPrice(const Order& incoming, int opposingPrice);

    std::vector<Trade> processBuyOrder(Order &newOrder);
    std::vector<Trade> processSellOrder(Order &newOrder);

    Order* getOrderById(uint64_t orderId);
    std::deque<uint64_t>* getOrderQueueByOrderId(uint64_t orderId);

    void saveOrderToBook(const Order& order);
};
