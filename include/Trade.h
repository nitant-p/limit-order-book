#pragma once
#include "Order.h"

struct Trade {
    uint64_t buyOrderId;
    uint64_t sellOrderId;
    int executionPrice;
    int executionQuantity;

    Trade(uint64_t b, uint64_t s, int p, int q)
        : buyOrderId(b), sellOrderId(s), executionPrice(p), executionQuantity(q) {}
};

std::ostream& operator<<(std::ostream& os, const  Trade& trade);
