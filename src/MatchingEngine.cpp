//
// Created by Nitant Panicker on 27/4/26.
//

#include "../include/MatchingEngine.h"

#include <iostream>
#include <limits>

using namespace std;

MatchingEngine::MatchingEngine(vector<Order> buyOrders,
                               vector<Order> sellOrders)
    : buyOrders(buyOrders), sellOrders(sellOrders) {
}

void MatchingEngine::processOrder(Order newOrder) {
    cout << "Processing order:" << newOrder;
    if (newOrder.side == Side::BUY) {
        // find best (lowest) sell order
        int minPrice = numeric_limits<int>::max();
        Order* minSellOrderPtr = nullptr;
        for (Order& o : this->sellOrders) {
            if (o.price <= newOrder.price and minPrice > o.price) { // match
                minSellOrderPtr = &o;
                minPrice = o.price;
            }
        }

        if (minSellOrderPtr != nullptr) {
            cout << "Best matching order is: " << *minSellOrderPtr << endl;
            this->processMatchedOrders(newOrder, *minSellOrderPtr);
        } else {
            cout << "No sell order was matched." << endl;
        }

        if (newOrder.quantity > 0) {
            this->buyOrders.push_back(newOrder);
        }

        removeEmptyOrders(Side::SELL);

    } else {
        // find best (highest) buy order
        Order* maxBuyOrderPtr = nullptr;
        int maxPrice = numeric_limits<int>::min();
        for (Order& o : this->buyOrders) {
            if (o.price >= newOrder.price and maxPrice < o.price) { // match
                maxBuyOrderPtr = &o;
                maxPrice = o.price;
            }
        }

        if (maxBuyOrderPtr != nullptr) {
            cout << "Best matching order is: " << *maxBuyOrderPtr << endl;
            this->processMatchedOrders(newOrder, *maxBuyOrderPtr);
        } else {
            cout << "No buy order was matched." << endl;
        }

        if (newOrder.quantity > 0) {
            this->sellOrders.push_back(newOrder);
        }

        removeEmptyOrders(Side::BUY);
    }

    this->printBook();

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
    
}

void MatchingEngine::removeEmptyOrders(Side side) {
    auto isEmpty = [](Order o) {
        return o.quantity == 0;
    };

    if (side == Side::BUY) {
        erase_if(this->buyOrders, isEmpty);
    } else {
        erase_if(this->sellOrders, isEmpty);
    }
}

void MatchingEngine::processMatchedOrders(Order& order1, Order& order2) {
    int minQuantity = min(order1.quantity, order2.quantity);
    cout << "Quantity traded: " << minQuantity << endl;
    order1.quantity -= minQuantity;
    order2.quantity -= minQuantity;
}

const vector<Order>& MatchingEngine::getBuyOrders() const {
    return this->buyOrders;
}

const vector<Order>& MatchingEngine::getSellOrders() const {
    return this->sellOrders;
}
