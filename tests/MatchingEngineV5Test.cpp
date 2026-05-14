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

TEST(MatchingEngineV5, SellSideStoresOrderIdsByPriceLevelFIFO) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 101, 5); // id 1
    engine.processOrder(Side::SELL, 101, 7); // id 2
    engine.processOrder(Side::SELL, 102, 3); // id 3

    const auto& sells = engine.getSellOrders();
    ASSERT_EQ(sells.size(), 2U);
    ASSERT_EQ(levelAt(sells, 101).size(), 2U);
    ASSERT_EQ(levelAt(sells, 102).size(), 1U);

    EXPECT_EQ(levelAt(sells, 101).at(0), 1U);
    EXPECT_EQ(levelAt(sells, 101).at(1), 2U);
    EXPECT_EQ(levelAt(sells, 102).at(0), 3U);
}

TEST(MatchingEngineV5, BuySideStoresOrderIdsByPriceLevelFIFO) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, 100, 4); // id 1
    engine.processOrder(Side::BUY, 100, 6); // id 2
    engine.processOrder(Side::BUY, 99, 2);  // id 3

    const auto& buys = engine.getBuyOrders();
    ASSERT_EQ(buys.size(), 2U);
    ASSERT_EQ(levelAt(buys, 100).size(), 2U);
    ASSERT_EQ(levelAt(buys, 99).size(), 1U);

    EXPECT_EQ(levelAt(buys, 100).at(0), 1U);
    EXPECT_EQ(levelAt(buys, 100).at(1), 2U);
    EXPECT_EQ(levelAt(buys, 99).at(0), 3U);
}

TEST(MatchingEngineV5, BuyOrderMatchesLowestAskPriceFirst) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 103, 5); // id 1
    engine.processOrder(Side::SELL, 101, 6); // id 2
    engine.processOrder(Side::SELL, 102, 7); // id 3

    const std::vector<Trade> trades = engine.processOrder(Side::BUY, 103, 4); // id 4

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades.at(0).buyOrderId, 4U);
    EXPECT_EQ(trades.at(0).sellOrderId, 2U);
    EXPECT_EQ(trades.at(0).executionPrice, 101);
    EXPECT_EQ(trades.at(0).executionQuantity, 4);

    const auto& sells = engine.getSellOrders();
    ASSERT_EQ(levelAt(sells, 101).size(), 1U);
    EXPECT_EQ(levelAt(sells, 101).front(), 2U);
}

TEST(MatchingEngineV5, SellOrderMatchesHighestBidPriceFirst) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, 99, 5);  // id 1
    engine.processOrder(Side::BUY, 101, 6); // id 2
    engine.processOrder(Side::BUY, 100, 7); // id 3

    const std::vector<Trade> trades = engine.processOrder(Side::SELL, 99, 4); // id 4

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades.at(0).buyOrderId, 2U);
    EXPECT_EQ(trades.at(0).sellOrderId, 4U);
    EXPECT_EQ(trades.at(0).executionPrice, 101);
    EXPECT_EQ(trades.at(0).executionQuantity, 4);


    const auto& buys = engine.getBuyOrders();
    ASSERT_EQ(levelAt(buys, 101).size(), 1U);
    EXPECT_EQ(levelAt(buys, 101).front(), 2U);
}

TEST(MatchingEngineV5, EmptyPriceLevelIsRemovedAfterDepletion) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 100, 5); // id 1
    const std::vector<Trade> trades = engine.processOrder(Side::BUY, 100, 5); // id 2

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_TRUE(engine.getSellOrders().empty());
}

TEST(MatchingEngineV5, MultiLevelMatchMaintainsPriceTimeSemantics) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 100, 3); // id 1
    engine.processOrder(Side::SELL, 100, 4); // id 2
    engine.processOrder(Side::SELL, 101, 5); // id 3

    const std::vector<Trade> trades = engine.processOrder(Side::BUY, 101, 9); // id 4

    ASSERT_EQ(trades.size(), 3U);
    EXPECT_EQ(trades.at(0).sellOrderId, 1U);
    EXPECT_EQ(trades.at(1).sellOrderId, 2U);
    EXPECT_EQ(trades.at(2).sellOrderId, 3U);

    const auto& sells = engine.getSellOrders();
    ASSERT_EQ(sells.size(), 1U);
    ASSERT_EQ(levelAt(sells, 101).size(), 1U);
    EXPECT_EQ(levelAt(sells, 101).front(), 3U);
}
