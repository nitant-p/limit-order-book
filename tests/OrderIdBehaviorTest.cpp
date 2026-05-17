#include "MatchingEngine.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <deque>
#include <map>
#include <vector>

namespace {

template <typename Comparator>
const std::deque<uint64_t>& levelAt(const std::map<int, std::deque<uint64_t>, Comparator>& levels, int price) {
    const auto it = levels.find(price);
    EXPECT_NE(it, levels.end());
    return it->second;
}

class OrderIdBehaviorTest : public ::testing::Test {
protected:
    MatchingEngine engine{{}, {}};
};

} // namespace

TEST_F(OrderIdBehaviorTest, EngineAssignsMonotonicIdsAcrossAllIncomingOrders) {
    engine.processOrder(Side::SELL, Type::LIMIT, 105, 5);  // id 1 rests
    engine.processOrder(Side::BUY, Type::MARKET, 0, 5);    // id 2 matched, not resting
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 3);   // id 3 rests

    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    ASSERT_EQ(levelAt(engine.getBuyOrders(), 100).size(), 1U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 100).front(), 3U);

    EXPECT_TRUE(engine.getSellOrders().empty());
}

TEST_F(OrderIdBehaviorTest, IdUniquenessPreservedAfterCancelAndNewOrder) {
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 5); // id 1
    const bool cancelled = engine.cancelOrder(1);
    EXPECT_TRUE(cancelled);

    engine.processOrder(Side::BUY, Type::LIMIT, 99, 4); // id 2

    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    ASSERT_EQ(levelAt(engine.getBuyOrders(), 99).size(), 1U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 99).front(), 2U);
}
