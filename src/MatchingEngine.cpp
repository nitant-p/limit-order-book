//
// Created by Nitant Panicker on 27/4/26.
//

#include "../include/MatchingEngine.h"

#include <iostream>
#include <limits>

using namespace std;

MatchingEngine::MatchingEngine(OrderBookSide buyBook,
                               OrderBookSide sellBook)
    : buyBook(buyBook), sellBook(sellBook) {
}

vector<Trade> MatchingEngine::processOrder(Side side, Type type, int price, int quantity) {
    Order newOrder {this->getAndIncrementNextOrderId(), side, type, price, quantity};
    cout << "Processing order:" << newOrder << endl;

    if (newOrder.side == Side::BUY) return this->processBuyOrder(newOrder);
    return this->processSellOrder(newOrder);
}

void MatchingEngine::printBook() {
    if (buyBook.empty()) {
        cout << "There are no buy orders." << endl;
    } else {
        cout << "These are the following buy orders:" << endl;
        vector<uint64_t> orderIds = buyBook.getAllOrderIds();
        for (auto it = orderIds.begin(); it != orderIds.end(); ++it) cout << this->idToOrderMap[*it] << endl;

    }

     if (sellBook.empty()) {
        cout << "There are no sell orders." << endl;
    } else {
        cout << "These are the following sell orders:" << endl;
        vector<uint64_t> orderIds = sellBook.getAllOrderIds();
        for (auto it = orderIds.begin(); it != orderIds.end(); ++it) cout << this->idToOrderMap[*it] << endl;

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
//         erase_if(this->buyBook, isEmpty);
//     } else {
//         erase_if(this->sellBook, isEmpty);
//     }
// }

Trade MatchingEngine::processMatchedOrders(
    Order& incomingOrder,
    const Order& restingOrder,
    uint64_t buyId,
    uint64_t sellId,
    OrderBookSide& book
    ) {
    const int minQuantity = min(incomingOrder.quantity, restingOrder.quantity);
    cout << "Quantity traded: " << minQuantity << endl;
    incomingOrder.quantity -= minQuantity;

    auto result = book.reduceOrderQuantity(restingOrder.id, minQuantity);
    // TODO: add exception if result is false

    const int priceTraded = restingOrder.price;
    cout << "Price traded at: " << priceTraded << endl;

    const Trade t(buyId, sellId, priceTraded,  minQuantity);
    tradeHistory.push_back(t);
    return t;
}

bool MatchingEngine::cancelOrder(uint64_t id) {
    if (!this->idToOrderMap.contains(id)) return false;

    Order& order = this->idToOrderMap[id];
    Side side = order.side;
    int price = order.price;

    deque<uint64_t>* queue =
    side == Side::BUY
        ? this->buyBook.findLevel(price)
        : this->sellBook.findLevel(price);

    for (auto it = queue->begin(); it != queue->end(); ++it) {
        if (*it == id) {
           this->idToOrderMap.erase(id);
           queue->erase(it);
           break;
        }
    }

    side == Side::BUY
        ? buyBook.removeLevelIfEmpty(price)
        : sellBook.removeLevelIfEmpty(price);

    return true;
}


const OrderBookSide& MatchingEngine::getBuyBook() const {
    return this->buyBook;
}

const OrderBookSide& MatchingEngine::getSellBook() const {
    return this->sellBook;
}

uint64_t MatchingEngine::getAndIncrementNextOrderId() {
    return this->nextOrderId++;
}

void MatchingEngine::deleteEmptyOrderInOrder(uint64_t orderId, deque<uint64_t>& orderQueue) {
    // TODO: fix
    this->idToOrderMap.erase(orderId);
    if (!orderQueue.empty()) orderQueue.pop_front();
}

void MatchingEngine::removeEmptyOrderQueuesByPrice(map<int, deque<uint64_t>>& map, int price) {
    if (map.count(price) and map[price].empty()) {
        map.erase(price);
    }
}

void MatchingEngine::addNewOrder(Side side, Order& newOrder) {
    uint64_t id = newOrder.id;
    if (side == Side::BUY) {
        this->buyBook.addOrder(id, newOrder.side, newOrder.type, newOrder.price, newOrder.quantity);
    } else {
        this->sellBook.addOrder(id, newOrder.side, newOrder.type, newOrder.price, newOrder.quantity);
    }
}


bool MatchingEngine::canContinueAgainstPrice(const Order& incoming, int opposingPrice) {
    if (incoming.type == Type::MARKET) return true;
    if (incoming.side == Side::BUY) return incoming.price >= opposingPrice;
    return incoming.price <= opposingPrice;
}

vector<Trade> MatchingEngine::processBuyOrder(Order &newOrder) {
    vector<Trade> tradeList;

    auto best = this->sellBook.bestPrice();
    if (!best) return tradeList;
    int bestSellPrice = *best;

    auto sellQueuePtr = this->sellBook.findLevel(bestSellPrice);

    while (newOrder.quantity > 0 and canContinueAgainstPrice(newOrder, bestSellPrice)) {
        const Order* currSellOrder = sellBook.getBestOrder();
        if (currSellOrder == nullptr) break;
        
        cout << "Matched with sell order: " << *currSellOrder << endl;
        Trade t = this->processMatchedOrders(newOrder, *currSellOrder, newOrder.id, currSellOrder->id, sellBook);
        tradeList.push_back(t);

        best = this->sellBook.bestPrice();
        if (!best) break;
        bestSellPrice = *best;
    }

    if (newOrder.type != Type::MARKET and newOrder.quantity > 0) this->addNewOrder(Side::BUY, newOrder);

    return tradeList;
}

vector<Trade> MatchingEngine::processSellOrder(Order &newOrder) {
    vector<Trade> tradeList;
    while (newOrder.quantity > 0 and !this->buyBook.empty()) {
        auto best = this->buyBook.bestPrice();
        if (!best) return;
        int bestBuyPrice = *best;

        if (!canContinueAgainstPrice(newOrder, bestBuyPrice)) break;

        deque<uint64_t>* buyQueue = this->buyBook.findLevel(bestBuyPrice);
        while (newOrder.quantity > 0 and buyQueue->empty()) {
            uint64_t id = buyQueue->front();
            Order& currBuyOrder = this->idToOrderMap[id];
            cout << "Matched with buy order: " << currBuyOrder << endl;
            Trade t = this->processMatchedOrders(newOrder, currBuyOrder,  currBuyOrder.id, newOrder.id);
            tradeList.push_back(t);

            if (currBuyOrder.quantity == 0) this->deleteEmptyOrderInOrder(id, *buyQueue);
        }

        buyBook.removeLevelIfEmpty(bestBuyPrice);
    }

    if (newOrder.type != Type::MARKET and newOrder.quantity > 0) this->addNewOrder(Side::SELL, newOrder);

    return tradeList;
}

bool MatchingEngine::modifyOrder(uint64_t orderId, int newPrice, int newQuantity) {
    Order* orderPtr = this->getOrderById(orderId);
    if (orderPtr == nullptr) {
        cout << "Order ID does not exist" << endl;
        return false;
    }

    if (newQuantity <= 0 or newPrice <= 0) {
        cout << "Quantity and price must be positive." << endl;
        return false;
    } else if (newQuantity == orderPtr->quantity and newPrice == orderPtr->price) {
        cout << "Order has not changed. Ignore request." << endl;
        return true;
    } else if (newQuantity < orderPtr->quantity and newPrice == orderPtr->price) {
        // can keep queue position
        cout << "Only quantity has reduced. Order can keep its queue position." << endl;
        orderPtr->quantity = newQuantity;
        return true;
    }

    cout << "Order must be reinserted into queue." << endl;

    deque<uint64_t>* queuePtr = this->getOrderQueueByOrderId(orderId);
    if (queuePtr == nullptr) {
        cout << "Could not find queue for this order." << endl;
        return false;
    }

    for (auto it = queuePtr->begin(); it != queuePtr->end(); ++it) {
        if (*it == orderId) {
            // delete from queue
            queuePtr->erase(it);
            break;
        }
    }

    // delete queue if empty
    if (orderPtr->side == Side::BUY) buyBook.removeLevelIfEmpty(orderPtr->price)
    else sellBook.removeLevelIfEmpty(orderPtr->price);


    // modify order
    orderPtr->price = newPrice;
    orderPtr->quantity = newQuantity;
    saveOrderToBook(*orderPtr);

    return true;
}

Order* MatchingEngine::getOrderById(uint64_t orderId) {
    auto it = this->idToOrderMap.find(orderId);
    if (it != this->idToOrderMap.end()) return &it->second;
    else return nullptr;
}

std::deque<uint64_t>* MatchingEngine::getOrderQueueByOrderId(uint64_t orderId) {
    Order* orderPtr = this->getOrderById(orderId);
    if (orderPtr == nullptr) {
        return nullptr;
    }

    OrderBookSide* orderBookPtr = nullptr;
    if (orderPtr->side == Side::BUY) orderBookPtr = &this->buyBook;
    else orderBookPtr = &this->sellBook;

    auto it = orderBookPtr->findLevel(orderPtr->price);
    return it;
}

void MatchingEngine::saveOrderToBook(const Order& order) {
    if (order.side == Side::BUY) this->buyBook.addOrder(order.price, order.id);
    else this->sellBook.addOrder(order.price, order.id);
}

