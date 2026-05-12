#include "MatchingEngine.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <deque>
#include <functional>
#include <map>

namespace {

template <typename Comparator>
const std::deque<Order>& levelAt(const std::map<int, std::deque<Order>, Comparator>& levels, int price) {
    const auto it = levels.find(price);
    EXPECT_NE(it, levels.end());
    return it->second;
}

void expectOrder(const Order& order, uint64_t id, Side side, int price, int quantity) {
    EXPECT_EQ(order.id, id);
    EXPECT_EQ(order.side, side);
    EXPECT_EQ(order.price, price);
    EXPECT_EQ(order.quantity, quantity);
}

} // namespace

TEST(MatchingEngineV5, SellSideIsGroupedByPriceLevelsWithFIFOQueues) {
    MatchingEngine engine(std::map<int, std::deque<Order>, std::greater<int>>{}, std::map<int, std::deque<Order>>{});

    engine.processOrder(Side::SELL, 101, 5);
    engine.processOrder(Side::SELL, 101, 7);
    engine.processOrder(Side::SELL, 102, 3);

    const auto& sells = engine.getSellOrders();

    ASSERT_EQ(sells.size(), 2U);
    ASSERT_EQ(levelAt(sells, 101).size(), 2U);
    ASSERT_EQ(levelAt(sells, 102).size(), 1U);

    expectOrder(levelAt(sells, 101).at(0), 1, Side::SELL, 101, 5);
    expectOrder(levelAt(sells, 101).at(1), 2, Side::SELL, 101, 7);
}

TEST(MatchingEngineV5, BuySideIsGroupedByPriceLevelsWithFIFOQueues) {
    MatchingEngine engine(std::map<int, std::deque<Order>, std::greater<int>>{}, std::map<int, std::deque<Order>>{});

    engine.processOrder(Side::BUY, 100, 4);
    engine.processOrder(Side::BUY, 100, 6);
    engine.processOrder(Side::BUY, 99, 2);

    const auto& buys = engine.getBuyOrders();

    ASSERT_EQ(buys.size(), 2U);
    ASSERT_EQ(levelAt(buys, 100).size(), 2U);
    ASSERT_EQ(levelAt(buys, 99).size(), 1U);

    expectOrder(levelAt(buys, 100).at(0), 1, Side::BUY, 100, 4);
    expectOrder(levelAt(buys, 100).at(1), 2, Side::BUY, 100, 6);
}

TEST(MatchingEngineV5, BuyOrderMatchesLowestAskPriceFirst) {
    MatchingEngine engine(std::map<int, std::deque<Order>, std::greater<int>>{}, std::map<int, std::deque<Order>>{});

    engine.processOrder(Side::SELL, 103, 5);
    engine.processOrder(Side::SELL, 101, 6);
    engine.processOrder(Side::SELL, 102, 7);

    const std::vector<Trade> trades = engine.processOrder(Side::BUY, 103, 4);

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades.at(0).executionPrice, 101);
    EXPECT_EQ(trades.at(0).executionQuantity, 4);

    const auto& sells = engine.getSellOrders();
    ASSERT_EQ(sells.size(), 3U);
    expectOrder(levelAt(sells, 101).front(), 2, Side::SELL, 101, 2);
}

TEST(MatchingEngineV5, SellOrderMatchesHighestBidPriceFirst) {
    MatchingEngine engine(std::map<int, std::deque<Order>, std::greater<int>>{}, std::map<int, std::deque<Order>>{});

    engine.processOrder(Side::BUY, 99, 5);
    engine.processOrder(Side::BUY, 101, 6);
    engine.processOrder(Side::BUY, 100, 7);

    const std::vector<Trade> trades = engine.processOrder(Side::SELL, 99, 4);

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades.at(0).executionPrice, 101);
    EXPECT_EQ(trades.at(0).executionQuantity, 4);

    const auto& buys = engine.getBuyOrders();
    ASSERT_EQ(buys.size(), 3U);
    expectOrder(levelAt(buys, 101).front(), 2, Side::BUY, 101, 2);
}

TEST(MatchingEngineV5, EmptyPriceLevelIsRemovedAfterDepletion) {
    MatchingEngine engine(std::map<int, std::deque<Order>, std::greater<int>>{}, std::map<int, std::deque<Order>>{});

    engine.processOrder(Side::SELL, 100, 5);
    const std::vector<Trade> trades = engine.processOrder(Side::BUY, 100, 5);

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_TRUE(engine.getSellOrders().empty());
}

TEST(MatchingEngineV5, MultiLevelMatchMaintainsPriceTimeSemantics) {
    MatchingEngine engine(std::map<int, std::deque<Order>, std::greater<int>>{}, std::map<int, std::deque<Order>>{});

    engine.processOrder(Side::SELL, 100, 3); // id 1
    engine.processOrder(Side::SELL, 100, 4); // id 2
    engine.processOrder(Side::SELL, 101, 5); // id 3

    const std::vector<Trade> trades = engine.processOrder(Side::BUY, 101, 9); // id 4

    ASSERT_EQ(trades.size(), 3U);

    EXPECT_EQ(trades.at(0).executionPrice, 100);
    EXPECT_EQ(trades.at(0).executionQuantity, 3);
    EXPECT_EQ(trades.at(0).sellOrderId, 1U);

    EXPECT_EQ(trades.at(1).executionPrice, 100);
    EXPECT_EQ(trades.at(1).executionQuantity, 4);
    EXPECT_EQ(trades.at(1).sellOrderId, 2U);

    EXPECT_EQ(trades.at(2).executionPrice, 101);
    EXPECT_EQ(trades.at(2).executionQuantity, 2);
    EXPECT_EQ(trades.at(2).sellOrderId, 3U);

    const auto& sells = engine.getSellOrders();
    ASSERT_EQ(sells.size(), 1U);
    expectOrder(levelAt(sells, 101).front(), 3, Side::SELL, 101, 3);
}
