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
    explicit MatchingEngine(std::size_t capacity);

    std::vector<Trade> processOrder(Side side, Type type, int price, int quantity);
    void printBook();
    const OrderBookSide& getBuyBook() const;
    const OrderBookSide& getSellOrders() const;
    bool cancelOrder(uint64_t cancelId);
    bool modifyOrder(uint64_t orderId, int newPrice, int newQuantity);
    
private:
    OrderNodePool sharedOrderPool_;
    uint64_t nextOrderId {1};

    OrderBookSide buyBook;
    OrderBookSide sellBook;
    std::vector<Trade> tradeHistory;
    
    std::unordered_map<uint64_t, Side> orderIdSide;

    // void removeEmptyOrders(Side side);
    Trade processMatchedOrders(Order& incomingOrder, const Order& restingOrder, uint64_t buyId, uint64_t sellId, OrderBookSide& book);
    uint64_t getAndIncrementNextOrderId();
    void addNewOrder(Side side, Order& newOrder);
    bool static canContinueAgainstPrice(const Order& incoming, int opposingPrice);

    std::vector<Trade> processBuyOrder(Order &newOrder);
    std::vector<Trade> processSellOrder(Order &newOrder);

    const Order* getOrderById(uint64_t orderId);

    void saveOrderId(uint64_t id, Side side);
};
