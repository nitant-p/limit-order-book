//
// Created by Nitant Panicker on 28/4/26.
//

#include "../include/Order.h"

#include <ostream>

std::string sideToString(Side side) {
    return side == Side::BUY ? "BUY" : "SELL";
}

std::ostream& operator<<(std::ostream& os, const Order& order) {
    os << "Order{id=" << order.id
       << ", side=" << sideToString(order.side)
       << ", price=" << order.price
       << ", quantity=" << order.quantity
       << "}";

    return os;
}

std::ostream& operator<<(std::ostream& os, Side side) {
    os << sideToString(side);
    return os;
}
