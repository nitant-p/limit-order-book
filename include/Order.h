#pragma once
#include <ostream>
enum class Side {BUY, SELL };

struct Order {
    int id;
    Side side;
    int price;
    int quantity;
};

std::ostream& operator<<(std::ostream& os, const Order& order);
std::ostream& operator<<(std::ostream& os, Side side);