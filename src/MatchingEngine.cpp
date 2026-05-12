//
// Created by Nitant Panicker on 27/4/26.
//

#include "../include/MatchingEngine.h"

#include <iostream>
#include <limits>

using namespace std;

MatchingEngine::MatchingEngine(map<int, deque<Order>, greater<int>> buyOrders,
                               map<int, deque<Order>> sellOrders)
    : buyOrders(buyOrders), sellOrders(sellOrders) {
}

vector<Trade> MatchingEngine::processOrder(Side side, int price, int quantity) {
    Order newOrder {this->getAndIncrementNextOrderId(), side, price, quantity};
    cout << "Processing order:" << newOrder;
    
    vector<Trade> newOrderTradeList;
    if (newOrder.side == Side::BUY) {

        auto it = this->sellOrders.begin();
        auto endIt = this->sellOrders.end();

        while (newOrder.quantity > 0 and it != endIt and canOrderPricesMatch(newOrder.price, it->first)) {
            deque<Order>& orderQueue = it->second;
            while (newOrder.quantity > 0 and !orderQueue.empty()) {
                Order& currSellOrder = orderQueue.front();
                cout << "Matched with sell order: " << currSellOrder << endl;
                Trade t = this->processMatchedOrders(newOrder, currSellOrder, newOrder.id, currSellOrder.id);
                newOrderTradeList.push_back(t);
                this->removeEmptyOrdersFromQueue(orderQueue);
            }

            if (orderQueue.empty()) {
                it = this->sellOrders.erase(it);
            } else {
                ++it;
            }
        }

        if (newOrder.quantity > 0) this->addNewOrder(Side::BUY, newOrder);

    } else {

        auto it = this->buyOrders.begin();
        auto endIt = this->buyOrders.end();

        while (newOrder.quantity > 0 and it != endIt and canOrderPricesMatch(it->first, newOrder.price)) {
            deque<Order>& orderQueue = it->second;
            while (newOrder.quantity > 0 and !orderQueue.empty()) {
                Order& currBuyOrder = orderQueue.front();
                cout << "Matched with buy order: " << currBuyOrder << endl;
                Trade t = this->processMatchedOrders(newOrder, currBuyOrder, newOrder.id, currBuyOrder.id);
                newOrderTradeList.push_back(t);
                this->removeEmptyOrdersFromQueue(orderQueue);
            }

            if (orderQueue.empty()) {
                it = this->buyOrders.erase(it);
            } else {
                ++it;
            }
        }

        if (newOrder.quantity > 0) this->addNewOrder(Side::SELL, newOrder);
    }

    return newOrderTradeList;
}

void MatchingEngine::printBook() {
    if (this->buyOrders.empty()) {
        cout << "There are no buy orders left." << endl;
    } else {
        cout << "These are the following buy orders:" << endl;
        for (auto it = this->buyOrders.begin(); it != buyOrders.begin(); ++it) {
            for (auto o : it->second) {
                cout << o << endl;
            }
        }
    }

    if (this->sellOrders.empty()) {
        cout << "There are no sell orders left." << endl;
    } else {
        cout << "These are the following sell orders:" << endl;
        for (auto it = this->sellOrders.begin(); it != sellOrders.begin(); ++it) {
            for (auto o : it->second) {
                cout << o << endl;
            }
        }
    }

    if (this->tradeHistory.empty()) {
        cout << "No trades have been made." << endl;
    } else {
        cout << "These trades have been made: " << endl;
        for (auto t : this->tradeHistory) {
            cout << t << endl;
        }
    }
    
}

// void MatchingEngine::removeEmptyOrders(Side side) {
//     auto isEmpty = [](Order o) {
//         return o.quantity == 0;
//     };

//     if (side == Side::BUY) {
//         erase_if(this->buyOrders, isEmpty);
//     } else {
//         erase_if(this->sellOrders, isEmpty);
//     }
// }

Trade MatchingEngine::processMatchedOrders(
    Order& incomingOrder,
    Order& restingOrder,
    uint64_t buyId,
    uint64_t sellId
    ) {
    const int minQuantity = min(incomingOrder.quantity, restingOrder.quantity);
    cout << "Quantity traded: " << minQuantity << endl;
    incomingOrder.quantity -= minQuantity;
    restingOrder.quantity -= minQuantity;

    const int priceTraded = restingOrder.price;
    cout << "Price traded at: " << priceTraded << endl;

    const Trade t(buyId, sellId, priceTraded,  minQuantity);
    tradeHistory.push_back(t);
    return t;
}

void MatchingEngine::cancelOrder(uint64_t cancelId) {
    Order& order = this->idToOrderMap[cancelId];
    int price = order.price;

    deque<Order>* orderQueuePtr = nullptr;

    if (this->buyOrders.contains(price)) {
        orderQueuePtr = &this->buyOrders[price];
    } else if (this->sellOrders.contains(price)) {
        orderQueuePtr = &this->sellOrders[price];
    }

    if (orderQueuePtr != nullptr and !orderQueuePtr->empty()) {
        for (auto it = orderQueuePtr->begin(); it < orderQueuePtr->end();) {
            if (it->id == cancelId) {
                it = orderQueuePtr->erase(it);
            }
            else {
                ++it;
            }
        }
        if (orderQueuePtr->empty())
    }
}


const map<int, deque<Order>, std::greater<int>>& MatchingEngine::getBuyOrders() const {
    return this->buyOrders;
}

const map<int, deque<Order>>& MatchingEngine::getSellOrders() const {
    return this->sellOrders;
}

uint64_t MatchingEngine::getAndIncrementNextOrderId() {
    return this->nextOrderId++;
}

void MatchingEngine::removeEmptyOrdersFromQueue(deque<Order>& orderQueue) {
    while (!orderQueue.empty()) {
        if (orderQueue.front().quantity == 0) {
            orderQueue.pop_front();
        } else {
            return;
        }
    }
}

void MatchingEngine::removeEmptyOrderQueuesByPrice(map<int, deque<Order>>& map, int price) {
    if (map.count(price) and map[price].empty()) {
        map.erase(price);
    }
}

bool MatchingEngine::canOrderPricesMatch(const int buyOrderPrice, const int sellOrderPrice) {
    return buyOrderPrice >= sellOrderPrice;
}

void MatchingEngine::addNewOrder(Side side, Order& newOrder) {
    if (side == Side::BUY) {
        this->buyOrders[newOrder.price].push_back(newOrder);
    } else {
        this->buyOrders[newOrder.price].push_back(newOrder);
    }

    this->idToOrderMap[newOrder.id] = newOrder;
}