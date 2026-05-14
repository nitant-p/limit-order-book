//
// Created by Nitant Panicker on 27/4/26.
//

#include "../include/MatchingEngine.h"

#include <iostream>
#include <limits>

using namespace std;

MatchingEngine::MatchingEngine(map<int, deque<uint64_t>> buyOrders,
                               map<int, deque<uint64_t>> sellOrders)
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
            deque<uint64_t>& orderIdQueue = it->second;
            while (newOrder.quantity > 0 and !orderIdQueue.empty()) {
                uint64_t id = orderIdQueue.front();
                Order& currSellOrder = this->idToOrderMap[id];
                cout << "Matched with sell order: " << currSellOrder << endl;
                Trade t = this->processMatchedOrders(newOrder, currSellOrder, newOrder.id, currSellOrder.id);
                newOrderTradeList.push_back(t);
                
                if (currSellOrder.quantity == 0) this->deleteEmptyOrderInOrder(id, orderIdQueue);
            }

            if (orderIdQueue.empty()) {
                it = this->sellOrders.erase(it);
            } else {
                ++it;
            }
        }

        if (newOrder.quantity > 0) this->addNewOrder(Side::BUY, newOrder);

    } else {
        // iterate backwards for highest price first
        auto endIt = this->buyOrders.end();
        auto startIt = this->buyOrders.begin();

        while (newOrder.quantity > 0 and startIt != startIt and canOrderPricesMatch(endIt->first, newOrder.price)) {
            --endIt;
            deque<uint64_t>& orderIdQueue = endIt->second;
            while (newOrder.quantity > 0 and !orderIdQueue.empty()) {
                uint64_t id = orderIdQueue.front();
                Order& currBuyOrder = this->idToOrderMap[id];
                cout << "Matched with buy order: " << currBuyOrder << endl;
                Trade t = this->processMatchedOrders(newOrder, currBuyOrder, newOrder.id, currBuyOrder.id);
                newOrderTradeList.push_back(t);
                this->deleteEmptyOrderInOrder(id, orderIdQueue);
            }

            if (orderIdQueue.empty()) {
                endIt = this->buyOrders.erase(endIt);
            }
        }

        if (newOrder.quantity > 0) this->addNewOrder(Side::SELL, newOrder);
    }

    return newOrderTradeList;
}

void MatchingEngine::printBook() {
    if (this->getBuyOrders().empty()) {
        cout << "There are no buy orders left." << endl;
    } else {
        cout << "These are the following buy orders:" << endl;
        for (auto it = this->getBuyOrders().begin(); it != this->getBuyOrders().end(); ++it) {
            for (auto id : it->second) {
                cout << this->idToOrderMap[id] << endl;
            }
        }
    }

    if (this->getSellOrders().empty()) {
        cout << "There are no sell orders left." << endl;
    } else {
        cout << "These are the following sell orders:" << endl;
        for (auto it = this->getSellOrders().begin(); it != this->getSellOrders().end(); ++it) {
            for (auto id : it->second) {
                cout << this->idToOrderMap[id] << endl;
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

void MatchingEngine::cancelOrder(uint64_t id) {
    if (!this-idToOrderMap.contains(id)) return;

    Order& order = this->idToOrderMap[id];
    Side side = order.side;
    int price = order.price;

    deque<uint64_t>& queue =
    side == Side::BUY
        ? this->buyOrders[price]
        : this->sellOrders[price];

    for (auto it = queue.begin(); it != queue.end(); ++it) {
        if (*it == id) {
           this->idToOrderMap.erase(id);
           queue.erase(it);
           break;
        }
    }

    if (queue.empty()) 

}


const map<int, deque<uint64_t>>& MatchingEngine::getBuyOrders() const {
    return this->buyOrders;
}

const map<int, deque<uint64_t>>& MatchingEngine::getSellOrders() const {
    return this->sellOrders;
}

uint64_t MatchingEngine::getAndIncrementNextOrderId() {
    return this->nextOrderId++;
}

void MatchingEngine::deleteEmptyOrderInOrder(uint64_t orderId, deque<uint64_t>& orderQueue) {
    this->idToOrderMap.erase(orderId);
    if (!orderQueue.empty()) orderQueue.pop_front();
}

void MatchingEngine::removeEmptyOrderQueuesByPrice(map<int, deque<uint64_t>>& map, int price) {
    if (map.count(price) and map[price].empty()) {
        map.erase(price);
    }
}

bool MatchingEngine::canOrderPricesMatch(const int buyOrderPrice, const int sellOrderPrice) {
    return buyOrderPrice >= sellOrderPrice;
}

void MatchingEngine::addNewOrder(Side side, Order& newOrder) {
    uint64_t id = newOrder.id;
    this->idToOrderMap[id] = newOrder;
    if (side == Side::BUY) {
        this->buyOrders[newOrder.price].push_back(id);
    } else {
        this->sellOrders[newOrder.price].push_back(id);
    }
}