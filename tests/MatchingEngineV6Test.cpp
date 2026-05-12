#include "MatchingEngine.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <deque>
#include <functional>
#include <map>

namespace {

using BuyLevels = std::map<int, std::deque<Order>, std::greater<int>>;
using SellLevels = std::map<int, std::deque<Order>>;

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

// V6 Requirement: cancel existing resting order by id -> success + order removed.
TEST(MatchingEngineV6, CancelExistingBuyOrder_ReturnsTrueAndRemovesOrder) {
    MatchingEngine engine(BuyLevels{}, SellLevels{});

    engine.processOrder(Side::BUY, 100, 10); // id=1
    ASSERT_EQ(engine.getBuyOrders().size(), 1U);

    const bool cancelled = engine.cancelOrder(1);

    EXPECT_TRUE(cancelled);
    EXPECT_TRUE(engine.getBuyOrders().empty());
}

// V6 Requirement: cancel existing resting sell order in same price queue preserves FIFO for remainder.
TEST(MatchingEngineV6, CancelExistingSellOrder_SamePriceQueueRemainingOrderPreserved) {
    MatchingEngine engine(BuyLevels{}, SellLevels{});

    engine.processOrder(Side::SELL, 101, 5); // id=1
    engine.processOrder(Side::SELL, 101, 7); // id=2

    const bool cancelled = engine.cancelOrder(1);

    EXPECT_TRUE(cancelled);
    ASSERT_EQ(engine.getSellOrders().size(), 1U);
    ASSERT_EQ(levelAt(engine.getSellOrders(), 101).size(), 1U);
    expectOrder(levelAt(engine.getSellOrders(), 101).front(), 2, Side::SELL, 101, 7);
}

// V6 Requirement: cancel missing id -> failure and no book mutation.
TEST(MatchingEngineV6, CancelMissingOrder_ReturnsFalseAndBookUnchanged) {
    MatchingEngine engine(BuyLevels{}, SellLevels{});

    engine.processOrder(Side::BUY, 100, 4);  // id=1
    engine.processOrder(Side::SELL, 103, 6); // id=2

    const auto beforeBuys = engine.getBuyOrders();
    const auto beforeSells = engine.getSellOrders();

    const bool cancelled = engine.cancelOrder(999);

    EXPECT_FALSE(cancelled);
    EXPECT_EQ(engine.getBuyOrders(), beforeBuys);
    EXPECT_EQ(engine.getSellOrders(), beforeSells);
}

// V6 Requirement: cancel after partial fill removes the remaining resting quantity.
TEST(MatchingEngineV6, CancelAfterPartialFill_ReturnsTrueAndRemovesRemainder) {
    MatchingEngine engine(BuyLevels{}, SellLevels{});

    engine.processOrder(Side::SELL, 100, 10); // id=1
    engine.processOrder(Side::BUY, 100, 4);   // id=2, partially fills id=1 -> id=1 left qty=6

    ASSERT_EQ(engine.getSellOrders().size(), 1U);
    expectOrder(levelAt(engine.getSellOrders(), 100).front(), 1, Side::SELL, 100, 6);

    const bool cancelled = engine.cancelOrder(1);

    EXPECT_TRUE(cancelled);
    EXPECT_TRUE(engine.getSellOrders().empty());
}

// V6 Requirement: cancel already fully-filled order must fail (not found in resting book).
TEST(MatchingEngineV6, CancelFullyFilledOrder_ReturnsFalse) {
    MatchingEngine engine(BuyLevels{}, SellLevels{});

    engine.processOrder(Side::SELL, 100, 5); // id=1
    engine.processOrder(Side::BUY, 100, 5);  // id=2, fully fills id=1

    ASSERT_TRUE(engine.getSellOrders().empty());

    const bool cancelled = engine.cancelOrder(1);

    EXPECT_FALSE(cancelled);
}

// V6 Requirement: removing the last order at a level removes that price level.
TEST(MatchingEngineV6, CancelLastOrderAtPriceLevel_RemovesThatLevelOnly) {
    MatchingEngine engine(BuyLevels{}, SellLevels{});

    engine.processOrder(Side::BUY, 101, 3); // id=1
    engine.processOrder(Side::BUY, 100, 2); // id=2

    ASSERT_EQ(engine.getBuyOrders().size(), 2U);

    const bool cancelled = engine.cancelOrder(1);

    EXPECT_TRUE(cancelled);
    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    EXPECT_EQ(engine.getBuyOrders().count(101), 0U);
    EXPECT_EQ(engine.getBuyOrders().count(100), 1U);
    expectOrder(levelAt(engine.getBuyOrders(), 100).front(), 2, Side::BUY, 100, 2);
}
