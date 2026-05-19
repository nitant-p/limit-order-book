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

class BestMatchingBySideTest : public ::testing::Test {
protected:
    MatchingEngine engine{OrderBookSide{Side::BUY}, OrderBookSide{Side::SELL}};
};

} // namespace

TEST_F(BestMatchingBySideTest, BuyLimitMatchesLowestAskPriceFirst) {
    engine.processOrder(Side::SELL, Type::LIMIT, 103, 5); // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 101, 6); // id 2
    engine.processOrder(Side::SELL, Type::LIMIT, 102, 7); // id 3

    const std::vector<Trade> trades = engine.processOrder(Side::BUY, Type::LIMIT, 103, 4); // id 4

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades.at(0).buyOrderId, 4U);
    EXPECT_EQ(trades.at(0).sellOrderId, 2U);
    EXPECT_EQ(trades.at(0).executionPrice, 101);
    EXPECT_EQ(trades.at(0).executionQuantity, 4);

    ASSERT_EQ(levelAt(engine.getSellBook(), 101).size(), 1U);
    EXPECT_EQ(levelAt(engine.getSellBook(), 101).front(), 2U);
}

TEST_F(BestMatchingBySideTest, SellLimitMatchesHighestBidPriceFirst) {
    engine.processOrder(Side::BUY, Type::LIMIT, 99, 5);  // id 1
    engine.processOrder(Side::BUY, Type::LIMIT, 101, 6); // id 2
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 7); // id 3

    const std::vector<Trade> trades = engine.processOrder(Side::SELL, Type::LIMIT, 99, 4); // id 4

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades.at(0).buyOrderId, 2U);
    EXPECT_EQ(trades.at(0).sellOrderId, 4U);
    EXPECT_EQ(trades.at(0).executionPrice, 101);
    EXPECT_EQ(trades.at(0).executionQuantity, 4);

    ASSERT_EQ(levelAt(engine.getBuyBook(), 101).size(), 1U);
    EXPECT_EQ(levelAt(engine.getBuyBook(), 101).front(), 2U);
}

TEST_F(BestMatchingBySideTest, BuySweepHonorsPriceTimePriorityAcrossLevels) {
    engine.processOrder(Side::SELL, Type::LIMIT, 100, 3); // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 100, 4); // id 2
    engine.processOrder(Side::SELL, Type::LIMIT, 101, 5); // id 3

    const std::vector<Trade> trades = engine.processOrder(Side::BUY, Type::LIMIT, 101, 9); // id 4

    ASSERT_EQ(trades.size(), 3U);
    EXPECT_EQ(trades.at(0).sellOrderId, 1U);
    EXPECT_EQ(trades.at(1).sellOrderId, 2U);
    EXPECT_EQ(trades.at(2).sellOrderId, 3U);

    const auto& sells = engine.getSellBook();
    ASSERT_EQ(sells.priceLevelCount(), 1U);
    ASSERT_EQ(levelAt(sells, 101).size(), 1U);
    EXPECT_EQ(levelAt(sells, 101).front(), 3U);
}

TEST_F(BestMatchingBySideTest, SellSweepHonorsPriceTimePriorityAcrossLevels) {
    engine.processOrder(Side::BUY, Type::LIMIT, 101, 2); // id 1
    engine.processOrder(Side::BUY, Type::LIMIT, 101, 3); // id 2
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 4); // id 3

    const std::vector<Trade> trades = engine.processOrder(Side::SELL, Type::LIMIT, 100, 7); // id 4

    ASSERT_EQ(trades.size(), 3U);
    EXPECT_EQ(trades.at(0).buyOrderId, 1U);
    EXPECT_EQ(trades.at(1).buyOrderId, 2U);
    EXPECT_EQ(trades.at(2).buyOrderId, 3U);

    ASSERT_EQ(engine.getBuyBook().priceLevelCount(), 1U);
    ASSERT_EQ(levelAt(engine.getBuyBook(), 100).size(), 1U);
    EXPECT_EQ(levelAt(engine.getBuyBook(), 100).front(), 3U);
}

TEST_F(BestMatchingBySideTest, LimitOrderStopsWhenBestOpposingPriceNoLongerCrosses) {
    engine.processOrder(Side::SELL, Type::LIMIT, 100, 2); // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 102, 3); // id 2

    const std::vector<Trade> trades = engine.processOrder(Side::BUY, Type::LIMIT, 100, 5); // id 3

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades.at(0).executionPrice, 100);
    EXPECT_EQ(trades.at(0).executionQuantity, 2);

    ASSERT_EQ(engine.getBuyBook().priceLevelCount(), 1U);
    ASSERT_EQ(levelAt(engine.getBuyBook(), 100).size(), 1U);
    EXPECT_EQ(levelAt(engine.getBuyBook(), 100).front(), 3U);

    ASSERT_EQ(engine.getSellBook().priceLevelCount(), 1U);
    ASSERT_EQ(levelAt(engine.getSellBook(), 102).size(), 1U);
    EXPECT_EQ(levelAt(engine.getSellBook(), 102).front(), 2U);
}

TEST_F(BestMatchingBySideTest, ExactFillRemovesDepletedPriceLevel) {
    engine.processOrder(Side::SELL, Type::LIMIT, 100, 5); // id 1

    const std::vector<Trade> trades = engine.processOrder(Side::BUY, Type::LIMIT, 100, 5); // id 2

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_TRUE(engine.getSellBook().empty());
    EXPECT_TRUE(engine.getBuyBook().empty());
}

TEST_F(BestMatchingBySideTest, NoMatchStoresIncomingOrderInOwnBook) {
    const std::vector<Trade> trades = engine.processOrder(Side::BUY, Type::LIMIT, 95, 7); // id 1

    EXPECT_TRUE(trades.empty());
    ASSERT_EQ(engine.getBuyBook().priceLevelCount(), 1U);
    ASSERT_EQ(levelAt(engine.getBuyBook(), 95).size(), 1U);
    EXPECT_EQ(levelAt(engine.getBuyBook(), 95).front(), 1U);
    EXPECT_TRUE(engine.getSellBook().empty());
}
