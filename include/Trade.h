#pragma once
#include "Order.h"

struct Trade {
    int buyOrderId;
    int sellOrderId;
    int executionPrice;
    int executionQuantity;

    Trade(int b, int s, int p, int q)
        : buyOrderId(b), sellOrderId(s), executionPrice(p), executionQuantity(q) {}
};

std::ostream& operator<<(std::ostream& os, const  Trade& trade);
