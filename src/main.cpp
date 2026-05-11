#include "MatchingEngine.h"

#include <vector>

int main() {
    std::vector<Order> buyOrders;
    std::vector<Order> sellOrders;

    MatchingEngine engine(buyOrders, sellOrders);

    engine.processOrder(Side::BUY, 100, 10);
    engine.processOrder(Side::SELL, 105, 5);
    engine.processOrder(Side::SELL, 99, 4);

    engine.printBook();

    return 0;
}
