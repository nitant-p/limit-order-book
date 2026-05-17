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

TEST(MatchingEngineV8Modify, ModifyMissingOrder_ReturnsFalseAndDoesNotMutateBook) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, Type::LIMIT, 100, 5); // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 105, 6); // id 2

    const auto beforeBuys = engine.getBuyOrders();
    const auto beforeSells = engine.getSellOrders();

    const bool ok = engine.modifyOrder(999, 101, 10);

    EXPECT_FALSE(ok);
    EXPECT_EQ(engine.getBuyOrders(), beforeBuys);
    EXPECT_EQ(engine.getSellOrders(), beforeSells);
}

TEST(MatchingEngineV8Modify, ReduceQuantitySamePrice_KeepsFIFOPosition) {
    MatchingEngine engine({}, {});

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

TEST(MatchingEngineV8Modify, IncreaseQuantitySamePrice_LosesPriorityAndMovesToBack) {
    MatchingEngine engine({}, {});

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

TEST(MatchingEngineV8Modify, PriceChange_MovesOrderToNewPriceLevel) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, Type::LIMIT, 100, 5); // id 1

    const bool ok = engine.modifyOrder(1, 102, 5);
    EXPECT_TRUE(ok);

    EXPECT_EQ(engine.getBuyOrders().count(100), 0U);
    EXPECT_EQ(engine.getBuyOrders().count(102), 1U);
    ASSERT_EQ(levelAt(engine.getBuyOrders(), 102).size(), 1U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 102).front(), 1U);
}

TEST(MatchingEngineV8Modify, PriceChangeToExistingLevel_AppendsAtBackOfThatLevel) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, Type::LIMIT, 100, 5); // id 1
    engine.processOrder(Side::BUY, Type::LIMIT, 101, 5); // id 2
    engine.processOrder(Side::BUY, Type::LIMIT, 101, 5); // id 3

    const bool ok = engine.modifyOrder(1, 101, 5);
    EXPECT_TRUE(ok);

    EXPECT_EQ(engine.getBuyOrders().count(100), 0U);
    ASSERT_EQ(levelAt(engine.getBuyOrders(), 101).size(), 3U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 101).at(0), 2U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 101).at(1), 3U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 101).at(2), 1U);
}

TEST(MatchingEngineV8Modify, ModifyFilledOrder_ReturnsFalse) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, Type::LIMIT, 100, 3); // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 100, 3); // id 2 fills id 1

    const bool ok = engine.modifyOrder(1, 101, 3);
    EXPECT_FALSE(ok);
}

TEST(MatchingEngineV8Modify, ModifyOneSide_DoesNotMutateOppositeSide) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, Type::LIMIT, 100, 5); // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 110, 7); // id 2

    const auto beforeSells = engine.getSellOrders();

    const bool ok = engine.modifyOrder(1, 101, 5);
    EXPECT_TRUE(ok);

    EXPECT_EQ(engine.getSellOrders(), beforeSells);
}
