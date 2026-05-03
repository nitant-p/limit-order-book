#include "MatchingEngine.h"

#include <vector>

int main() {
    std::vector<Order> buyOrders;
    std::vector<Order> sellOrders;

    MatchingEngine engine(buyOrders, sellOrders);

    engine.processOrder(Order{1, Side::BUY, 100, 10});
    engine.processOrder(Order{2, Side::SELL, 105, 5});
    engine.processOrder(Order{3, Side::SELL, 99, 4});

    engine.printBook();

    return 0;
}
