#include "MatchingEngine.h"

#include <gtest/gtest.h>
#include <deque>
#include <functional>
#include <map>

namespace {

void expectOrder(const Order& order, uint64_t id, Side side, int price, int quantity) {
    EXPECT_EQ(order.id, id);
    EXPECT_EQ(order.side, side);
    EXPECT_EQ(order.price, price);
    EXPECT_EQ(order.quantity, quantity);
}

void expectTrade(const Trade& trade, uint64_t buyOrderId, uint64_t sellOrderId, int executionPrice, int executionQuantity) {
    EXPECT_EQ(trade.buyOrderId, buyOrderId);
    EXPECT_EQ(trade.sellOrderId, sellOrderId);
    EXPECT_EQ(trade.executionPrice, executionPrice);
    EXPECT_EQ(trade.executionQuantity, executionQuantity);
}

} // namespace

TEST(MatchingEngineV1, NoMatch_BuyOrderAddedToBook) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, 100, 10);

    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    ASSERT_TRUE(engine.getSellOrders().empty());
    expectOrder(engine.getBuyOrders().front(), 1, Side::BUY, 100, 10);
}

TEST(MatchingEngineV1, ExactMatch_BothOrdersRemoved) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 100, 10);
    engine.processOrder(Side::BUY, 100, 10);

    EXPECT_TRUE(engine.getBuyOrders().empty());
    EXPECT_TRUE(engine.getSellOrders().empty());
}

TEST(MatchingEngineV1, PartialFill_RestingOrderRemains) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 100, 20);
    engine.processOrder(Side::BUY, 100, 5);

    EXPECT_TRUE(engine.getBuyOrders().empty());
    ASSERT_EQ(engine.getSellOrders().size(), 1U);
    expectOrder(engine.getSellOrders().front(), 1, Side::SELL, 100, 15);
}

TEST(MatchingEngineV1, IncomingLeftover_StoredInOwnBook) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 100, 5);
    engine.processOrder(Side::BUY, 100, 20);

    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    EXPECT_TRUE(engine.getSellOrders().empty());
    expectOrder(engine.getBuyOrders().front(), 2, Side::BUY, 100, 15);
}

TEST(MatchingEngineV1, OnlyOneMatchPerIncomingOrder) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 100, 5);
    engine.processOrder(Side::SELL, 99, 7);
    engine.processOrder(Side::BUY, 101, 20);

    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    expectOrder(engine.getBuyOrders().front(), 3, Side::BUY, 101, 13);

    ASSERT_EQ(engine.getSellOrders().size(), 1U);
    expectOrder(engine.getSellOrders().front(), 1, Side::SELL, 100, 5);
}

TEST(MatchingEngineV1, SellSideMatchingRule_WorksSymmetrically) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, 100, 8);
    engine.processOrder(Side::SELL, 99, 3);

    EXPECT_TRUE(engine.getSellOrders().empty());
    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    expectOrder(engine.getBuyOrders().front(), 1, Side::BUY, 100, 5);
}

TEST(MatchingEngineV2, BuySelectsLowestCompatibleSell) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 103, 10);
    engine.processOrder(Side::SELL, 101, 20);
    engine.processOrder(Side::SELL, 102, 15);

    engine.processOrder(Side::BUY, 102, 20);

    EXPECT_TRUE(engine.getBuyOrders().empty());
    ASSERT_EQ(engine.getSellOrders().size(), 2U);
    expectOrder(engine.getSellOrders().at(0), 1, Side::SELL, 103, 10);
    expectOrder(engine.getSellOrders().at(1), 3, Side::SELL, 102, 15);
}

TEST(MatchingEngineV2, SellSelectsHighestCompatibleBuy) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, 97, 10);
    engine.processOrder(Side::BUY, 99, 20);
    engine.processOrder(Side::BUY, 98, 15);

    engine.processOrder(Side::SELL, 98, 12);

    EXPECT_TRUE(engine.getSellOrders().empty());
    ASSERT_EQ(engine.getBuyOrders().size(), 3U);
    expectOrder(engine.getBuyOrders().at(0), 1, Side::BUY, 97, 10);
    expectOrder(engine.getBuyOrders().at(1), 2, Side::BUY, 99, 8);
    expectOrder(engine.getBuyOrders().at(2), 3, Side::BUY, 98, 15);
}

TEST(MatchingEngineV2, NoCompatibleSell_BuyRestsInBook) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 105, 7);
    engine.processOrder(Side::SELL, 106, 9);
    engine.processOrder(Side::BUY, 100, 4);

    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    expectOrder(engine.getBuyOrders().front(), 3, Side::BUY, 100, 4);
    ASSERT_EQ(engine.getSellOrders().size(), 2U);
}

TEST(MatchingEngineV2, EqualBestPriceUsesFirstSeenOrder) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 100, 5);
    engine.processOrder(Side::SELL, 100, 7);
    engine.processOrder(Side::BUY, 100, 6);

    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    expectOrder(engine.getBuyOrders().front(), 3, Side::BUY, 100, 1);
    ASSERT_EQ(engine.getSellOrders().size(), 1U);
    expectOrder(engine.getSellOrders().front(), 2, Side::SELL, 100, 7);
}

TEST(MatchingEngineV3, BuySweepsMultipleSellLevels_StopsAtPriceLimit) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 100, 30);
    engine.processOrder(Side::SELL, 101, 50);
    engine.processOrder(Side::SELL, 102, 40);

    engine.processOrder(Side::BUY, 101, 100);

    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    expectOrder(engine.getBuyOrders().front(), 4, Side::BUY, 101, 20);

    ASSERT_EQ(engine.getSellOrders().size(), 1U);
    expectOrder(engine.getSellOrders().front(), 3, Side::SELL, 102, 40);
}

TEST(MatchingEngineV3, SellSweepsMultipleBuyLevels_StopsAtPriceLimit) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, 105, 25);
    engine.processOrder(Side::BUY, 103, 30);
    engine.processOrder(Side::BUY, 102, 30);

    engine.processOrder(Side::SELL, 103, 70);

    ASSERT_EQ(engine.getSellOrders().size(), 1U);
    expectOrder(engine.getSellOrders().front(), 4, Side::SELL, 103, 15);

    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    expectOrder(engine.getBuyOrders().front(), 3, Side::BUY, 102, 30);
}

TEST(MatchingEngineV3, ExactFillAcrossMultipleLevels_NoIncomingRest) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 99, 10);
    engine.processOrder(Side::SELL, 100, 15);

    engine.processOrder(Side::BUY, 100, 25);

    EXPECT_TRUE(engine.getBuyOrders().empty());
    EXPECT_TRUE(engine.getSellOrders().empty());
}

TEST(MatchingEngineV3, PartialFillOnLastLevel_RestingOrderKeepsRemainder) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 100, 10);
    engine.processOrder(Side::SELL, 101, 50);

    engine.processOrder(Side::BUY, 101, 30);

    EXPECT_TRUE(engine.getBuyOrders().empty());
    ASSERT_EQ(engine.getSellOrders().size(), 1U);
    expectOrder(engine.getSellOrders().front(), 2, Side::SELL, 101, 30);
}

TEST(MatchingEngineV3, NoCompatibleOpposingOrder_StoresIncoming) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 105, 10);
    engine.processOrder(Side::BUY, 100, 7);

    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    expectOrder(engine.getBuyOrders().front(), 2, Side::BUY, 100, 7);
    ASSERT_EQ(engine.getSellOrders().size(), 1U);
    expectOrder(engine.getSellOrders().front(), 1, Side::SELL, 105, 10);
}

TEST(MatchingEngineV3, BestPriceChosenEachIteration_NotFirstFound) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 102, 5);
    engine.processOrder(Side::SELL, 100, 5);
    engine.processOrder(Side::SELL, 101, 5);

    engine.processOrder(Side::BUY, 102, 15);

    EXPECT_TRUE(engine.getBuyOrders().empty());
    EXPECT_TRUE(engine.getSellOrders().empty());
}

TEST(MatchingEngineV3, ReturnedTrades_BuySweepSequenceIsCorrect) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 100, 30);
    engine.processOrder(Side::SELL, 101, 50);
    engine.processOrder(Side::SELL, 102, 40);

    const std::vector<Trade> trades = engine.processOrder(Side::BUY, 101, 100);

    ASSERT_EQ(trades.size(), 2U);
    expectTrade(trades.at(0), 4, 1, 100, 30);
    expectTrade(trades.at(1), 4, 2, 101, 50);
}

TEST(MatchingEngineV3, ReturnedTrades_SellSweepSequenceIsCorrect) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, 105, 25);
    engine.processOrder(Side::BUY, 103, 30);
    engine.processOrder(Side::BUY, 102, 30);

    const std::vector<Trade> trades = engine.processOrder(Side::SELL, 103, 70);

    ASSERT_EQ(trades.size(), 2U);
    expectTrade(trades.at(0), 1, 4, 105, 25);
    expectTrade(trades.at(1), 2, 4, 103, 30);
}

TEST(MatchingEngineV3, ReturnedTrades_NoMatchReturnsEmptyList) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 105, 10);
    const std::vector<Trade> trades = engine.processOrder(Side::BUY, 100, 7);

    EXPECT_TRUE(trades.empty());
}

TEST(MatchingEngineV4, BuySideEqualPrice_UsesFIFOTimePriority) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 100, 10); // earlier
    engine.processOrder(Side::SELL, 100, 20); // later

    const std::vector<Trade> trades = engine.processOrder(Side::BUY, 100, 15);

    ASSERT_EQ(trades.size(), 2U);
    expectTrade(trades.at(0), 3, 1, 100, 10);
    expectTrade(trades.at(1), 3, 2, 100, 5);
}

TEST(MatchingEngineV4, SellSideEqualPrice_UsesFIFOTimePriority) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, 100, 8);  // earlier
    engine.processOrder(Side::BUY, 100, 9);  // later

    const std::vector<Trade> trades = engine.processOrder(Side::SELL, 100, 12);

    ASSERT_EQ(trades.size(), 2U);
    expectTrade(trades.at(0), 1, 3, 100, 8);
    expectTrade(trades.at(1), 2, 3, 100, 4);
}

TEST(MatchingEngineV4, BestPriceFirst_ThenFIFOWithinPriceLevel) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 99, 5);   // earlier at best price
    engine.processOrder(Side::SELL, 100, 7);  // worse price
    engine.processOrder(Side::SELL, 99, 6);   // later at best price

    const std::vector<Trade> trades = engine.processOrder(Side::BUY, 100, 10);

    ASSERT_EQ(trades.size(), 2U);
    expectTrade(trades.at(0), 4, 1, 99, 5);
    expectTrade(trades.at(1), 4, 3, 99, 5);
}
