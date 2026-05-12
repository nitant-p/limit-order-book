//
// Created by Nitant Panicker on 27/4/26.
//

#include "../include/MatchingEngine.h"

#include <iostream>
#include <limits>

using namespace std;

MatchingEngine::MatchingEngine(map<int, deque<Order>> buyOrders,
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

        while (newOrder.quantity > 0 and it != endIt and newOrder.price >= it->first) {
            deque<Order>& orderQueue = it->second;
            while (newOrder.quantity > 0 and !orderQueue.empty()) {
                Order& currSellOrder = orderQueue.front();
                cout << "Matched with sell order: " << currSellOrder << endl;
                Trade t = this->processMatchedOrders(newOrder, currSellOrder, newOrder.id, currSellOrder.id);
                newOrderTradeList.push_back(t);
                this->removeEmptyOrdersFromQueue(orderQueue);
            }
        }

        if (newOrder.quantity > 0) this->buyOrders[newOrder.price].push_back(newOrder);

    } else {

        auto it = this->buyOrders.begin();
        auto endIt = this->buyOrders.end();

        while (newOrder.quantity > 0 and it != endIt and it->first >= newOrder.price) {
            deque<Order>& orderQueue = it->second;
            while (newOrder.quantity > 0 and !orderQueue.empty()) {
                Order& currBuyOrder = orderQueue.front();
                cout << "Matched with buy order: " << currBuyOrder << endl;
                Trade t = this->processMatchedOrders(newOrder, currBuyOrder, newOrder.id, currBuyOrder.id);
                newOrderTradeList.push_back(t);
                this->removeEmptyOrdersFromQueue(orderQueue);
            }

        }

        if (newOrder.quantity > 0) this->sellOrders[newOrder.price].push_back(newOrder);

    }

    return newOrderTradeList;
}

void MatchingEngine::printBook() {
    if (this->buyOrders.empty()) {
        cout << "There are no buy orders left." << endl;
    } else {
        cout << "These are the following buy orders:" << endl;
        for (auto o : this->buyOrders) {
            cout << o << endl;
        }
    }

    if (this->sellOrders.empty()) {
        cout << "There are no sell orders left." << endl;
    } else {
        cout << "These are the following sell orders:" << endl;
        for (auto o : this->sellOrders) {
            cout << o << endl;
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

const vector<Order>& MatchingEngine::getBuyOrders() const {
    return this->buyOrders;
}

const vector<Order>& MatchingEngine::getSellOrders() const {
    return this->sellOrders;
}

uint64_t MatchingEngine::getAndIncrementNextOrderId() {
    return this->nextOrderId++;
}

void removeEmptyOrdersFromQueue(deque<Order>& orderQueue) {
    while (!orderQueue.empty()) {
        if (orderQueue.front().quantity == 0) {
            orderQueue.pop_front();
        } else {
            return;
        }
    }
}

void removeEmptyOrderQueuesByPrice(map<int, deque<Order>>& map, int price) {
    if (thi)
}