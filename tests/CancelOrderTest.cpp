#include "MatchingEngine.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <deque>
#include <map>

namespace {

template <typename Comparator>
const std::deque<uint64_t>& levelAt(const std::map<int, std::deque<uint64_t>, Comparator>& levels, int price) {
    const auto it = levels.find(price);
    EXPECT_NE(it, levels.end());
    return it->second;
}

class CancelOrderTest : public ::testing::Test {
protected:
    MatchingEngine engine{{}, {}};
};

} // namespace

TEST_F(CancelOrderTest, CancelMissingOrderReturnsFalseAndDoesNotMutateBook) {
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 4);  // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 103, 6); // id 2

    const auto beforeBuys = engine.getBuyOrders();
    const auto beforeSells = engine.getSellOrders();

    const bool ok = engine.cancelOrder(999);

    EXPECT_FALSE(ok);
    EXPECT_EQ(engine.getBuyOrders(), beforeBuys);
    EXPECT_EQ(engine.getSellOrders(), beforeSells);
}

TEST_F(CancelOrderTest, CancelExistingBuyOrderRemovesLastOrderAtPriceLevel) {
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 10); // id 1

    const bool ok = engine.cancelOrder(1);

    EXPECT_TRUE(ok);
    EXPECT_TRUE(engine.getBuyOrders().empty());
}

TEST_F(CancelOrderTest, CancelExistingSellOrderRemovesOnlyTargetFromPriceQueue) {
    engine.processOrder(Side::SELL, Type::LIMIT, 101, 5); // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 101, 7); // id 2

    const bool ok = engine.cancelOrder(1);

    EXPECT_TRUE(ok);
    ASSERT_EQ(engine.getSellOrders().size(), 1U);
    ASSERT_EQ(levelAt(engine.getSellOrders(), 101).size(), 1U);
    EXPECT_EQ(levelAt(engine.getSellOrders(), 101).front(), 2U);
}

TEST_F(CancelOrderTest, CancelMiddleOrderPreservesRemainingFIFOOrder) {
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 2); // id 1
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 3); // id 2
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 4); // id 3

    const bool ok = engine.cancelOrder(2);

    EXPECT_TRUE(ok);
    ASSERT_EQ(levelAt(engine.getBuyOrders(), 100).size(), 2U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 100).at(0), 1U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 100).at(1), 3U);
}

TEST_F(CancelOrderTest, CancelTwiceSecondCallReturnsFalseWithNoFurtherMutation) {
    engine.processOrder(Side::BUY, Type::LIMIT, 101, 3); // id 1
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 2); // id 2

    const bool first = engine.cancelOrder(1);
    const auto afterFirstBuys = engine.getBuyOrders();
    const auto afterFirstSells = engine.getSellOrders();

    const bool second = engine.cancelOrder(1);

    EXPECT_TRUE(first);
    EXPECT_FALSE(second);
    EXPECT_EQ(engine.getBuyOrders(), afterFirstBuys);
    EXPECT_EQ(engine.getSellOrders(), afterFirstSells);
}

TEST_F(CancelOrderTest, CancelPartiallyFilledRestingOrderRemovesItsRemainder) {
    engine.processOrder(Side::SELL, Type::LIMIT, 100, 10); // id 1
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 4);   // id 2

    ASSERT_EQ(engine.getSellOrders().size(), 1U);
    ASSERT_EQ(levelAt(engine.getSellOrders(), 100).size(), 1U);
    EXPECT_EQ(levelAt(engine.getSellOrders(), 100).front(), 1U);

    const bool ok = engine.cancelOrder(1);

    EXPECT_TRUE(ok);
    EXPECT_TRUE(engine.getSellOrders().empty());
}

TEST_F(CancelOrderTest, CancelDoesNotAffectOppositeSideBook) {
    engine.processOrder(Side::BUY, Type::LIMIT, 99, 2);   // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 105, 7); // id 2
    engine.processOrder(Side::SELL, Type::LIMIT, 106, 8); // id 3

    const auto beforeSells = engine.getSellOrders();

    const bool ok = engine.cancelOrder(1);

    EXPECT_TRUE(ok);
    EXPECT_EQ(engine.getSellOrders(), beforeSells);
    EXPECT_TRUE(engine.getBuyOrders().empty());
}
