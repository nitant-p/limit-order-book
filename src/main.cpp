#include "MatchingEngine.h"

#include <vector>

int main() {
    std::map<int, std::deque<uint64_t>> buyOrders;
    std::map<int, std::deque<uint64_t>> sellBook;

    MatchingEngine engine(buyOrders, sellBook);

    engine.processOrder(Side::BUY, Type::LIMIT, 100, 10);
    engine.processOrder(Side::SELL, Type::LIMIT, 105, 5);
    engine.processOrder(Side::SELL, Type::LIMIT, 99, 4);

    engine.printBook();

    return 0;
}
