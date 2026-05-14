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

} // namespace

TEST(MatchingEngineV7, MarketBuyConsumesBestAsksAcrossPriceLevels) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, Type::LIMIT, 100, 3); // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 101, 4); // id 2
    engine.processOrder(Side::SELL, Type::LIMIT, 102, 5); // id 3

    const std::vector<Trade> trades = engine.processOrder(Side::BUY, Type::MARKET, 0, 7); // id 4

    ASSERT_EQ(trades.size(), 2U);
    EXPECT_EQ(trades.at(0).buyOrderId, 4U);
    EXPECT_EQ(trades.at(0).sellOrderId, 1U);
    EXPECT_EQ(trades.at(0).executionPrice, 100);
    EXPECT_EQ(trades.at(0).executionQuantity, 3);

    EXPECT_EQ(trades.at(1).buyOrderId, 4U);
    EXPECT_EQ(trades.at(1).sellOrderId, 2U);
    EXPECT_EQ(trades.at(1).executionPrice, 101);
    EXPECT_EQ(trades.at(1).executionQuantity, 4);

    ASSERT_EQ(engine.getSellOrders().size(), 1U);
    EXPECT_EQ(engine.getSellOrders().count(102), 1U);
    ASSERT_EQ(levelAt(engine.getSellOrders(), 102).size(), 1U);
    EXPECT_EQ(levelAt(engine.getSellOrders(), 102).front(), 3U);
}

TEST(MatchingEngineV7, MarketSellConsumesBestBidsAcrossPriceLevels) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, Type::LIMIT, 101, 5); // id 1
    engine.processOrder(Side::BUY, Type::LIMIT, 103, 2); // id 2
    engine.processOrder(Side::BUY, Type::LIMIT, 102, 3); // id 3

    const std::vector<Trade> trades = engine.processOrder(Side::SELL, Type::MARKET, 0, 4); // id 4

    ASSERT_EQ(trades.size(), 2U);
    EXPECT_EQ(trades.at(0).buyOrderId, 2U);
    EXPECT_EQ(trades.at(0).sellOrderId, 4U);
    EXPECT_EQ(trades.at(0).executionPrice, 103);
    EXPECT_EQ(trades.at(0).executionQuantity, 2);

    EXPECT_EQ(trades.at(1).buyOrderId, 3U);
    EXPECT_EQ(trades.at(1).sellOrderId, 4U);
    EXPECT_EQ(trades.at(1).executionPrice, 102);
    EXPECT_EQ(trades.at(1).executionQuantity, 2);

    ASSERT_EQ(engine.getBuyOrders().size(), 2U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 101).size(), 1U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 101).front(), 1U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 102).size(), 1U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 102).front(), 3U);
}

TEST(MatchingEngineV7, MarketBuyIgnoresPriceConstraintField) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, Type::LIMIT, 105, 2); // id 1

    const std::vector<Trade> trades = engine.processOrder(Side::BUY, Type::MARKET, 1, 2); // id 2

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades.at(0).buyOrderId, 2U);
    EXPECT_EQ(trades.at(0).sellOrderId, 1U);
    EXPECT_EQ(trades.at(0).executionPrice, 105);
    EXPECT_EQ(trades.at(0).executionQuantity, 2);
    EXPECT_TRUE(engine.getSellOrders().empty());
}

TEST(MatchingEngineV7, MarketSellIgnoresPriceConstraintField) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, Type::LIMIT, 95, 3); // id 1

    const std::vector<Trade> trades = engine.processOrder(Side::SELL, Type::MARKET, 9999, 2); // id 2

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades.at(0).buyOrderId, 1U);
    EXPECT_EQ(trades.at(0).sellOrderId, 2U);
    EXPECT_EQ(trades.at(0).executionPrice, 95);
    EXPECT_EQ(trades.at(0).executionQuantity, 2);

    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 95).size(), 1U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 95).front(), 1U);
}

TEST(MatchingEngineV7, MarketBuyWithNoLiquidity_DoesNotRestInBook) {
    MatchingEngine engine({}, {});

    const std::vector<Trade> trades = engine.processOrder(Side::BUY, Type::MARKET, 0, 10); // id 1

    EXPECT_TRUE(trades.empty());
    EXPECT_TRUE(engine.getBuyOrders().empty());
    EXPECT_TRUE(engine.getSellOrders().empty());
}

TEST(MatchingEngineV7, MarketSellWithNoLiquidity_DoesNotRestInBook) {
    MatchingEngine engine({}, {});

    const std::vector<Trade> trades = engine.processOrder(Side::SELL, Type::MARKET, 0, 10); // id 1

    EXPECT_TRUE(trades.empty());
    EXPECT_TRUE(engine.getBuyOrders().empty());
    EXPECT_TRUE(engine.getSellOrders().empty());
}

TEST(MatchingEngineV7, MarketBuyWithInsufficientLiquidity_UnfilledRemainderIsDiscarded) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, Type::LIMIT, 100, 2); // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 101, 3); // id 2

    const std::vector<Trade> trades = engine.processOrder(Side::BUY, Type::MARKET, 0, 10); // id 3

    ASSERT_EQ(trades.size(), 2U);
    EXPECT_EQ(trades.at(0).executionQuantity, 2);
    EXPECT_EQ(trades.at(1).executionQuantity, 3);

    EXPECT_TRUE(engine.getSellOrders().empty());
    EXPECT_TRUE(engine.getBuyOrders().empty());
}

TEST(MatchingEngineV7, MarketSellWithInsufficientLiquidity_UnfilledRemainderIsDiscarded) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, Type::LIMIT, 101, 2); // id 1
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 3); // id 2

    const std::vector<Trade> trades = engine.processOrder(Side::SELL, Type::MARKET, 0, 10); // id 3

    ASSERT_EQ(trades.size(), 2U);
    EXPECT_EQ(trades.at(0).executionQuantity, 2);
    EXPECT_EQ(trades.at(1).executionQuantity, 3);

    EXPECT_TRUE(engine.getBuyOrders().empty());
    EXPECT_TRUE(engine.getSellOrders().empty());
}

TEST(MatchingEngineV7, LimitOrderBehaviorRemainsUnchanged) {
    MatchingEngine engine({}, {});

    const std::vector<Trade> trades = engine.processOrder(Side::BUY, Type::LIMIT, 100, 4); // id 1

    EXPECT_TRUE(trades.empty());
    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    ASSERT_EQ(levelAt(engine.getBuyOrders(), 100).size(), 1U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 100).front(), 1U);
}
