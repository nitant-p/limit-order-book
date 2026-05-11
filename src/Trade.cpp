#include "Trade.h"

std::ostream& operator<<(std::ostream& os, const Trade& trade) {
    os << "Trade{buyOrderId=" << trade.buyOrderId
       << ", sellOrderId=" << trade.sellOrderId
       << ", executionPrice=" << trade.executionPrice
       << ", executionQuantity=" << trade.executionQuantity
       << "}";

    return os;
}
