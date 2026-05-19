#include "MatchingEngine.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <deque>
#include <vector>

namespace {

const std::deque<uint64_t>& levelAt(const OrderBookSide& side, int price) {
    const auto* queue = side.findLevel(price);
    EXPECT_NE(queue, nullptr);
    return *queue;
}

class ModifyOrderTest : public ::testing::Test {
protected:
    MatchingEngine engine{OrderBookSide{Side::BUY}, OrderBookSide{Side::SELL}};
};

} // namespace

TEST_F(ModifyOrderTest, ModifyMissingOrderReturnsFalseAndDoesNotMutateBook) {
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 5);  // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 105, 6); // id 2

    const auto beforeBuy100 = levelAt(engine.getBuyOrders(), 100);
    const auto beforeSell105 = levelAt(engine.getSellBook(), 105);

    const bool ok = engine.modifyOrder(999, 101, 10);

    EXPECT_FALSE(ok);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 100), beforeBuy100);
    EXPECT_EQ(levelAt(engine.getSellBook(), 105), beforeSell105);
}

TEST_F(ModifyOrderTest, ModifyRejectsNonPositivePriceOrQuantity) {
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 5); // id 1

    const auto beforeBuy100 = levelAt(engine.getBuyOrders(), 100);

    EXPECT_FALSE(engine.modifyOrder(1, 0, 5));
    EXPECT_FALSE(engine.modifyOrder(1, 100, 0));
    EXPECT_FALSE(engine.modifyOrder(1, -1, 5));
    EXPECT_FALSE(engine.modifyOrder(1, 100, -2));

    EXPECT_EQ(levelAt(engine.getBuyOrders(), 100), beforeBuy100);
}

TEST_F(ModifyOrderTest, ModifyNoOpReturnsTrueWithoutChangingBook) {
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 5); // id 1

    const auto beforeBuy100 = levelAt(engine.getBuyOrders(), 100);

    const bool ok = engine.modifyOrder(1, 100, 5);

    EXPECT_TRUE(ok);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 100), beforeBuy100);
}

TEST_F(ModifyOrderTest, ReduceQuantitySamePriceKeepsFIFOPosition) {
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 10); // id 1
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 10); // id 2

    const bool ok = engine.modifyOrder(1, 100, 5);
    EXPECT_TRUE(ok);

    const std::vector<Trade> trades = engine.processOrder(Side::SELL, Type::LIMIT, 100, 6); // id 3

    ASSERT_EQ(trades.size(), 2U);
    EXPECT_EQ(trades.at(0).buyOrderId, 1U);
    EXPECT_EQ(trades.at(0).executionQuantity, 5);
    EXPECT_EQ(trades.at(1).buyOrderId, 2U);
    EXPECT_EQ(trades.at(1).executionQuantity, 1);
}

TEST_F(ModifyOrderTest, IncreaseQuantitySamePriceLosesPriorityAndMovesToBack) {
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 5); // id 1
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 5); // id 2

    const bool ok = engine.modifyOrder(1, 100, 8);
    EXPECT_TRUE(ok);

    const std::vector<Trade> trades = engine.processOrder(Side::SELL, Type::LIMIT, 100, 6); // id 3

    ASSERT_EQ(trades.size(), 2U);
    EXPECT_EQ(trades.at(0).buyOrderId, 2U);
    EXPECT_EQ(trades.at(0).executionQuantity, 5);
    EXPECT_EQ(trades.at(1).buyOrderId, 1U);
    EXPECT_EQ(trades.at(1).executionQuantity, 1);
}

TEST_F(ModifyOrderTest, PriceChangeMovesOrderToNewPriceLevel) {
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 5); // id 1

    const bool ok = engine.modifyOrder(1, 102, 5);

    EXPECT_TRUE(ok);
    EXPECT_EQ(engine.getBuyOrders().findLevel(100), nullptr);
    EXPECT_NE(engine.getBuyOrders().findLevel(102), nullptr);
    ASSERT_EQ(levelAt(engine.getBuyOrders(), 102).size(), 1U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 102).front(), 1U);
}

TEST_F(ModifyOrderTest, PriceChangeToExistingLevelAppendsAtBackOfLevelQueue) {
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 5); // id 1
    engine.processOrder(Side::BUY, Type::LIMIT, 101, 5); // id 2
    engine.processOrder(Side::BUY, Type::LIMIT, 101, 5); // id 3

    const bool ok = engine.modifyOrder(1, 101, 5);

    EXPECT_TRUE(ok);
    EXPECT_EQ(engine.getBuyOrders().findLevel(100), nullptr);
    ASSERT_EQ(levelAt(engine.getBuyOrders(), 101).size(), 3U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 101).at(0), 2U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 101).at(1), 3U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 101).at(2), 1U);
}

TEST_F(ModifyOrderTest, ModifyFilledOrderReturnsFalse) {
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 3);  // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 100, 3); // id 2 fills id 1

    const bool ok = engine.modifyOrder(1, 101, 3);

    EXPECT_FALSE(ok);
}

TEST_F(ModifyOrderTest, ModifyOneSideDoesNotMutateOppositeSideBook) {
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 5);   // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 110, 7);  // id 2

    const auto beforeSell110 = levelAt(engine.getSellBook(), 110);

    const bool ok = engine.modifyOrder(1, 101, 5);

    EXPECT_TRUE(ok);
    EXPECT_EQ(levelAt(engine.getSellBook(), 110), beforeSell110);
}
