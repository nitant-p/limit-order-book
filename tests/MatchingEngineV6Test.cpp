#include "MatchingEngine.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <deque>
#include <map>
#include <type_traits>
#include <utility>

namespace {

template <typename Comparator>
const std::deque<uint64_t>& levelAt(const std::map<int, std::deque<uint64_t>, Comparator>& levels, int price) {
    const auto it = levels.find(price);
    EXPECT_NE(it, levels.end());
    return it->second;
}

} // namespace

TEST(MatchingEngineV6, CancelOrderShouldReturnBoolStatus) {
    constexpr bool returnsBool =
        std::is_same_v<decltype(std::declval<MatchingEngine&>().cancelOrder(uint64_t{1})), bool>;

    EXPECT_TRUE(returnsBool);
}

TEST(MatchingEngineV6, CancelOnEmptyBook_DoesNotMutate) {
    MatchingEngine engine({}, {});

    engine.cancelOrder(42);

    EXPECT_TRUE(engine.getBuyOrders().empty());
    EXPECT_TRUE(engine.getSellOrders().empty());
}

TEST(MatchingEngineV6, CancelExistingBuyOrder_RemovesOrderIdFromBook) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, 100, 10); // id 1
    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    ASSERT_EQ(levelAt(engine.getBuyOrders(), 100).size(), 1U);

    engine.cancelOrder(1);

    EXPECT_TRUE(engine.getBuyOrders().empty());
}

TEST(MatchingEngineV6, CancelExistingSellOrder_RemovesOrderIdFromBook) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 101, 5); // id 1
    ASSERT_EQ(engine.getSellOrders().size(), 1U);

    engine.cancelOrder(1);

    EXPECT_TRUE(engine.getSellOrders().empty());
}

TEST(MatchingEngineV6, CancelMiddleOrderAtSamePrice_RemovesOnlyTargetId) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, 100, 2); // id 1
    engine.processOrder(Side::BUY, 100, 3); // id 2
    engine.processOrder(Side::BUY, 100, 4); // id 3

    ASSERT_EQ(levelAt(engine.getBuyOrders(), 100).size(), 3U);

    engine.cancelOrder(2);

    ASSERT_EQ(levelAt(engine.getBuyOrders(), 100).size(), 2U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 100).at(0), 1U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 100).at(1), 3U);
}

TEST(MatchingEngineV6, CancelExistingSellOrder_SamePriceQueueRemainingIdPreserved) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 101, 5); // id 1
    engine.processOrder(Side::SELL, 101, 7); // id 2

    engine.cancelOrder(1);

    ASSERT_EQ(engine.getSellOrders().size(), 1U);
    ASSERT_EQ(levelAt(engine.getSellOrders(), 101).size(), 1U);
    EXPECT_EQ(levelAt(engine.getSellOrders(), 101).front(), 2U);
}

TEST(MatchingEngineV6, CancelMissingOrderId_DoesNotMutateBook) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, 100, 4);  // id 1
    engine.processOrder(Side::SELL, 103, 6); // id 2

    const auto beforeBuys = engine.getBuyOrders();
    const auto beforeSells = engine.getSellOrders();

    engine.cancelOrder(999);

    EXPECT_EQ(engine.getBuyOrders(), beforeBuys);
    EXPECT_EQ(engine.getSellOrders(), beforeSells);
}

TEST(MatchingEngineV6, CancelTwice_SecondCancelDoesNotMutateBook) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, 101, 3); // id 1
    engine.processOrder(Side::BUY, 100, 2); // id 2

    engine.cancelOrder(1);
    const auto afterFirstCancelBuys = engine.getBuyOrders();
    const auto afterFirstCancelSells = engine.getSellOrders();

    engine.cancelOrder(1);

    EXPECT_EQ(engine.getBuyOrders(), afterFirstCancelBuys);
    EXPECT_EQ(engine.getSellOrders(), afterFirstCancelSells);
}

TEST(MatchingEngineV6, CancelAfterPartialFill_RemovesRestingRemainder_SellSide) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 100, 10); // id 1
    engine.processOrder(Side::BUY, 100, 4);   // id 2, id 1 partially filled and still resting

    ASSERT_EQ(engine.getSellOrders().size(), 1U);
    ASSERT_EQ(levelAt(engine.getSellOrders(), 100).size(), 1U);
    EXPECT_EQ(levelAt(engine.getSellOrders(), 100).front(), 1U);

    engine.cancelOrder(1);

    EXPECT_TRUE(engine.getSellOrders().empty());
}

TEST(MatchingEngineV6, CancelAfterPartialFill_RemovesRestingRemainder_BuySide) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, 100, 9);   // id 1
    engine.processOrder(Side::SELL, 100, 4);  // id 2, id 1 partially filled and still resting

    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    ASSERT_EQ(levelAt(engine.getBuyOrders(), 100).size(), 1U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 100).front(), 1U);

    engine.cancelOrder(1);

    EXPECT_TRUE(engine.getBuyOrders().empty());
}

TEST(MatchingEngineV6, CancelFullyFilledOrderId_NoMutation) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::SELL, 100, 5); // id 1
    engine.processOrder(Side::BUY, 100, 5);  // id 2, id 1 fully filled

    ASSERT_TRUE(engine.getSellOrders().empty());
    const auto beforeBuys = engine.getBuyOrders();
    const auto beforeSells = engine.getSellOrders();

    engine.cancelOrder(1);

    EXPECT_EQ(engine.getBuyOrders(), beforeBuys);
    EXPECT_EQ(engine.getSellOrders(), beforeSells);
}

TEST(MatchingEngineV6, CancelLastOrderAtPriceLevel_RemovesOnlyThatLevel) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, 101, 3); // id 1
    engine.processOrder(Side::BUY, 100, 2); // id 2

    ASSERT_EQ(engine.getBuyOrders().size(), 2U);

    engine.cancelOrder(1);

    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    EXPECT_EQ(engine.getBuyOrders().count(101), 0U);
    EXPECT_EQ(engine.getBuyOrders().count(100), 1U);
    EXPECT_EQ(levelAt(engine.getBuyOrders(), 100).front(), 2U);
}

TEST(MatchingEngineV6, CancelDoesNotAffectOppositeSideBook) {
    MatchingEngine engine({}, {});

    engine.processOrder(Side::BUY, 99, 2);   // id 1
    engine.processOrder(Side::SELL, 105, 7); // id 2
    engine.processOrder(Side::SELL, 106, 8); // id 3

    const auto beforeSells = engine.getSellOrders();

    engine.cancelOrder(1);

    EXPECT_EQ(engine.getSellOrders(), beforeSells);
    EXPECT_TRUE(engine.getBuyOrders().empty());
}
