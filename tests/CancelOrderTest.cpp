#include "MatchingEngine.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <deque>

namespace {

const std::deque<uint64_t>& levelAt(const OrderBookSide& side, int price) {
    const auto* queue = side.findLevel(price);
    EXPECT_NE(queue, nullptr);
    return *queue;
}

class CancelOrderTest : public ::testing::Test {
protected:
    MatchingEngine engine{OrderBookSide{Side::BUY}, OrderBookSide{Side::SELL}};
};

} // namespace

TEST_F(CancelOrderTest, CancelMissingOrderReturnsFalseAndDoesNotMutateBook) {
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 4);  // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 103, 6); // id 2

    const size_t beforeBuyLevels = engine.getBuyOrders().priceLevelCount();
    const size_t beforeSellLevels = engine.getSellBook().priceLevelCount();
    const auto beforeBuy100 = levelAt(engine.getBuyOrders(), 100);
    const auto beforeSell103 = levelAt(engine.getSellBook(), 103);

    const bool ok = engine.cancelOrder(999);

    EXPECT_FALSE(ok);
    EXPECT_EQ(engine.getBuyOrders().priceLevelCount(), beforeBuyLevels);
    EXPECT_EQ(engine.getSellBook().priceLevelCount(), beforeSellLevels);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 100), beforeBuy100);
    EXPECT_EQ(levelAt(engine.getSellBook(), 103), beforeSell103);
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
    ASSERT_EQ(engine.getSellBook().priceLevelCount(), 1U);
    ASSERT_EQ(levelAt(engine.getSellBook(), 101).size(), 1U);
    EXPECT_EQ(levelAt(engine.getSellBook(), 101).front(), 2U);
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
    const auto afterFirstBuy100 = levelAt(engine.getBuyOrders(), 100);
    const size_t afterFirstBuyLevels = engine.getBuyOrders().priceLevelCount();

    const bool second = engine.cancelOrder(1);

    EXPECT_TRUE(first);
    EXPECT_FALSE(second);
    EXPECT_EQ(engine.getBuyOrders().priceLevelCount(), afterFirstBuyLevels);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 100), afterFirstBuy100);
}

TEST_F(CancelOrderTest, CancelPartiallyFilledRestingOrderRemovesItsRemainder) {
    engine.processOrder(Side::SELL, Type::LIMIT, 100, 10); // id 1
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 4);   // id 2

    ASSERT_EQ(engine.getSellBook().priceLevelCount(), 1U);
    ASSERT_EQ(levelAt(engine.getSellBook(), 100).size(), 1U);
    EXPECT_EQ(levelAt(engine.getSellBook(), 100).front(), 1U);

    const bool ok = engine.cancelOrder(1);

    EXPECT_TRUE(ok);
    EXPECT_TRUE(engine.getSellBook().empty());
}

TEST_F(CancelOrderTest, CancelDoesNotAffectOppositeSideBook) {
    engine.processOrder(Side::BUY, Type::LIMIT, 99, 2);   // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 105, 7); // id 2
    engine.processOrder(Side::SELL, Type::LIMIT, 106, 8); // id 3

    const auto beforeSell105 = levelAt(engine.getSellBook(), 105);
    const auto beforeSell106 = levelAt(engine.getSellBook(), 106);

    const bool ok = engine.cancelOrder(1);

    EXPECT_TRUE(ok);
    EXPECT_EQ(levelAt(engine.getSellBook(), 105), beforeSell105);
    EXPECT_EQ(levelAt(engine.getSellBook(), 106), beforeSell106);
    EXPECT_TRUE(engine.getBuyOrders().empty());
}
