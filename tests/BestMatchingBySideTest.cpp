#include "MatchingEngine.h"
#include "TestHelpers.h"

#include <gtest/gtest.h>

#include <vector>

namespace {

using test_helpers::levelIds;

class BestMatchingBySideTest : public ::testing::Test {
protected:
    MatchingEngine engine{1'000};
};

} // namespace

TEST_F(BestMatchingBySideTest, BuyLimitMatchesLowestAskPriceFirst) {
    engine.processOrder(Side::SELL, Type::LIMIT, 103, 5); // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 101, 6); // id 2
    engine.processOrder(Side::SELL, Type::LIMIT, 102, 7); // id 3

    const std::vector<Trade> trades = engine.processOrder(Side::BUY, Type::LIMIT, 103, 4); // id 4

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades[0].buyOrderId, 4U);
    EXPECT_EQ(trades[0].sellOrderId, 2U);
    EXPECT_EQ(trades[0].executionPrice, 101);
    EXPECT_EQ(trades[0].executionQuantity, 4);

    EXPECT_EQ(levelIds(engine.getSellOrders(), 101), std::vector<uint64_t>({2}));
}

TEST_F(BestMatchingBySideTest, SellLimitMatchesHighestBidPriceFirst) {
    engine.processOrder(Side::BUY, Type::LIMIT, 99, 5);  // id 1
    engine.processOrder(Side::BUY, Type::LIMIT, 101, 6); // id 2
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 7); // id 3

    const std::vector<Trade> trades = engine.processOrder(Side::SELL, Type::LIMIT, 99, 4); // id 4

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades[0].buyOrderId, 2U);
    EXPECT_EQ(trades[0].sellOrderId, 4U);
    EXPECT_EQ(trades[0].executionPrice, 101);
    EXPECT_EQ(trades[0].executionQuantity, 4);

    EXPECT_EQ(levelIds(engine.getBuyBook(), 101), std::vector<uint64_t>({2}));
}

TEST_F(BestMatchingBySideTest, BuySweepHonorsPriceTimePriorityAcrossLevels) {
    engine.processOrder(Side::SELL, Type::LIMIT, 100, 3); // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 100, 4); // id 2
    engine.processOrder(Side::SELL, Type::LIMIT, 101, 5); // id 3

    const std::vector<Trade> trades = engine.processOrder(Side::BUY, Type::LIMIT, 101, 9); // id 4

    ASSERT_EQ(trades.size(), 3U);
    EXPECT_EQ(trades[0].sellOrderId, 1U);
    EXPECT_EQ(trades[1].sellOrderId, 2U);
    EXPECT_EQ(trades[2].sellOrderId, 3U);

    EXPECT_EQ(levelIds(engine.getSellOrders(), 101), std::vector<uint64_t>({3}));
}

TEST_F(BestMatchingBySideTest, SellSweepHonorsPriceTimePriorityAcrossLevels) {
    engine.processOrder(Side::BUY, Type::LIMIT, 101, 2); // id 1
    engine.processOrder(Side::BUY, Type::LIMIT, 101, 3); // id 2
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 4); // id 3

    const std::vector<Trade> trades = engine.processOrder(Side::SELL, Type::LIMIT, 100, 7); // id 4

    ASSERT_EQ(trades.size(), 3U);
    EXPECT_EQ(trades[0].buyOrderId, 1U);
    EXPECT_EQ(trades[1].buyOrderId, 2U);
    EXPECT_EQ(trades[2].buyOrderId, 3U);

    EXPECT_EQ(levelIds(engine.getBuyBook(), 100), std::vector<uint64_t>({3}));
}

TEST_F(BestMatchingBySideTest, LimitOrderStopsWhenBestOpposingPriceNoLongerCrosses) {
    engine.processOrder(Side::SELL, Type::LIMIT, 100, 2); // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 102, 3); // id 2

    const std::vector<Trade> trades = engine.processOrder(Side::BUY, Type::LIMIT, 100, 5); // id 3

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades[0].executionPrice, 100);
    EXPECT_EQ(trades[0].executionQuantity, 2);

    EXPECT_EQ(levelIds(engine.getBuyBook(), 100), std::vector<uint64_t>({3}));
    EXPECT_EQ(levelIds(engine.getSellOrders(), 102), std::vector<uint64_t>({2}));
}

TEST_F(BestMatchingBySideTest, ExactFillRemovesDepletedPriceLevels) {
    engine.processOrder(Side::SELL, Type::LIMIT, 100, 5); // id 1

    const std::vector<Trade> trades = engine.processOrder(Side::BUY, Type::LIMIT, 100, 5); // id 2

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_TRUE(engine.getSellOrders().empty());
    EXPECT_TRUE(engine.getBuyBook().empty());
}

TEST_F(BestMatchingBySideTest, NoMatchStoresIncomingOrderInOwnBook) {
    const std::vector<Trade> trades = engine.processOrder(Side::BUY, Type::LIMIT, 95, 7); // id 1

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(levelIds(engine.getBuyBook(), 95), std::vector<uint64_t>({1}));
    EXPECT_TRUE(engine.getSellOrders().empty());
}
