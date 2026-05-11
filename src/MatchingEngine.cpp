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

vector<Trade> MatchingEngine::processOrder(Side side, int price, int quantity) {
    Order newOrder {this->getAndIncrementNextOrderId(), side, price, quantity};
    cout << "Processing order:" << newOrder;
    
    vector<Trade> newOrderTradeList;
    if (newOrder.side == Side::BUY) {
        // sort sell orders by ascending, begin with lowest price matching
        sort(this->sellOrders.begin(), this->sellOrders.end(), AscendingPrice());
        int orderIndex = 0;
        while (newOrder.quantity > 0 and orderIndex < this->sellOrders.size()) {
            Order& currSellOrder = this->sellOrders[orderIndex];
            if (currSellOrder.price > newOrder.price) break; // reached end of matchable orders

            cout << "Matched with sell order: " << currSellOrder << endl;
            Trade t = this->processMatchedOrders(newOrder, currSellOrder, newOrder.id, currSellOrder.id);
            newOrderTradeList.push_back(t);

            ++orderIndex;
        }

        if (newOrder.quantity > 0) {
            this->buyOrders.push_back(newOrder);
        }

        removeEmptyOrders(Side::SELL);

    } else {
        // sort buy orders by descending, begin with highest price matching
        sort(this->buyOrders.begin(), this->buyOrders.end(), DescendingPrice());
        int orderIndex = 0;
        while (newOrder.quantity > 0 and orderIndex < this->buyOrders.size()) {
            Order& currBuyOrder = this->buyOrders[orderIndex];
            if (newOrder.price > currBuyOrder.price) break; // reached end of matchable orders

            cout << "Matched with buy order: " << currBuyOrder << endl;
            Trade t =this->processMatchedOrders(newOrder, currBuyOrder, currBuyOrder.id, newOrder.id);
            newOrderTradeList.push_back(t);

            ++orderIndex;
        }

        if (newOrder.quantity > 0) {
            this->sellOrders.push_back(newOrder);
        }

        removeEmptyOrders(Side::BUY);
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
