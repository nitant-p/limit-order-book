#include "MatchingEngine.h"

#include <vector>

int main() {
    MatchingEngine engine{1'000};

    engine.processOrder(Side::BUY, Type::LIMIT, 100, 10);
    engine.processOrder(Side::SELL, Type::LIMIT, 105, 5);
    engine.processOrder(Side::SELL, Type::LIMIT, 99, 4);

    engine.printBook();

    return 0;
}
