#pragma once
#include <cstdint>
#include <ostream>
enum class Side {BUY, SELL };

struct Order {
    uint64_t id;
    Side side;
    int price;
    int quantity;
};

struct AscendingPrice {
    bool operator() (const Order& order1, const Order& order2);
};

struct DescendingPrice {
    bool operator() (const Order& order1, const Order& order2);
};


std::ostream& operator<<(std::ostream& os, const Order& order);
std::ostream& operator<<(std::ostream& os, Side side);
