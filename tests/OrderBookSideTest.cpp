#include "OrderBookSide.h"

#include <gtest/gtest.h>

#include <deque>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace {

std::unordered_map<uint64_t, Order> makeOrders() {
    return {
        {1, Order{1, Side::BUY, Type::LIMIT, 100, 10}},
        {2, Order{2, Side::BUY, Type::LIMIT, 100, 15}},
        {3, Order{3, Side::BUY, Type::LIMIT, 101, 5}},
        {4, Order{4, Side::SELL, Type::LIMIT, 103, 7}},
        {5, Order{5, Side::SELL, Type::LIMIT, 104, 9}},
        {6, Order{6, Side::SELL, Type::LIMIT, 104, 11}},
    };
}

class OrderBookSideBuyTest : public ::testing::Test {
protected:
    OrderBookSide side{Side::BUY};
};

class OrderBookSideSellTest : public ::testing::Test {
protected:
    OrderBookSide side{Side::SELL};
};

TEST(OrderBookSideContract, MethodSignaturesAreAsExpected) {
    static_assert(std::is_same_v<decltype(std::declval<const OrderBookSide&>().side()), Side>);
    static_assert(std::is_same_v<decltype(std::declval<const OrderBookSide&>().empty()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const OrderBookSide&>().priceLevelCount()), size_t>);
    static_assert(std::is_same_v<decltype(std::declval<const OrderBookSide&>().bestPrice()), std::optional<int>>);

    static_assert(std::is_same_v<decltype(std::declval<OrderBookSide&>().addOrder(100, uint64_t{1})), void>);

    static_assert(std::is_same_v<decltype(std::declval<const OrderBookSide&>().findLevel(100)), const std::deque<uint64_t>*>);
    static_assert(std::is_same_v<decltype(std::declval<OrderBookSide&>().findLevel(100)), std::deque<uint64_t>*>);

    static_assert(std::is_same_v<decltype(std::declval<OrderBookSide&>().removeEmptyLevel(100)), void>);
    static_assert(std::is_same_v<decltype(std::declval<const OrderBookSide&>().orderCountAtPrice(100)), size_t>);

    static_assert(std::is_same_v<
        decltype(std::declval<const OrderBookSide&>().volumeAtPrice(100, std::declval<const std::unordered_map<uint64_t, Order>&>())),
        uint64_t>
    );

    static_assert(std::is_same_v<
        decltype(std::declval<const OrderBookSide&>().getDepth(5, std::declval<const std::unordered_map<uint64_t, Order>&>())),
        std::vector<LevelSnapshot>>
    );
}

TEST_F(OrderBookSideBuyTest, EmptySideHasNoBestPriceAndNoLevels) {
    EXPECT_TRUE(side.empty());
    EXPECT_EQ(side.priceLevelCount(), 0U);
    EXPECT_FALSE(side.bestPrice().has_value());
    EXPECT_EQ(side.orderCountAtPrice(100), 0U);
}

TEST_F(OrderBookSideBuyTest, AddOrderCreatesLevelAndMaintainsFIFOWithinLevel) {
    side.addOrder(100, 1);
    side.addOrder(100, 2);

    const auto* queue = side.findLevel(100);
    ASSERT_NE(queue, nullptr);
    ASSERT_EQ(queue->size(), 2U);
    EXPECT_EQ(queue->at(0), 1U);
    EXPECT_EQ(queue->at(1), 2U);
    EXPECT_EQ(side.orderCountAtPrice(100), 2U);
}

TEST_F(OrderBookSideBuyTest, BestPriceForBuySideIsHighestPrice) {
    side.addOrder(100, 1);
    side.addOrder(101, 3);

    const auto best = side.bestPrice();
    ASSERT_TRUE(best.has_value());
    EXPECT_EQ(best.value(), 101);
}

TEST_F(OrderBookSideSellTest, BestPriceForSellSideIsLowestPrice) {
    side.addOrder(104, 5);
    side.addOrder(103, 4);

    const auto best = side.bestPrice();
    ASSERT_TRUE(best.has_value());
    EXPECT_EQ(best.value(), 103);
}

TEST_F(OrderBookSideBuyTest, VolumeAtPriceSumsQuantitiesFromAuthoritativeOrderMap) {
    const auto ordersById = makeOrders();

    side.addOrder(100, 1);
    side.addOrder(100, 2);

    EXPECT_EQ(side.volumeAtPrice(100, ordersById), 25U);
}

TEST_F(OrderBookSideBuyTest, VolumeAtPriceReturnsZeroForMissingLevelOrEmptyLevel) {
    const auto ordersById = makeOrders();

    EXPECT_EQ(side.volumeAtPrice(100, ordersById), 0U);

    side.addOrder(100, 1);
    auto* queue = side.findLevel(100);
    ASSERT_NE(queue, nullptr);
    queue->clear();
    side.removeEmptyLevel(100);

    EXPECT_EQ(side.volumeAtPrice(100, ordersById), 0U);
}

TEST_F(OrderBookSideBuyTest, GetDepthForBuyReturnsHighestToLowestPrices) {
    const auto ordersById = makeOrders();

    side.addOrder(100, 1);
    side.addOrder(100, 2);
    side.addOrder(101, 3);

    const std::vector<LevelSnapshot> depth = side.getDepth(2, ordersById);
    ASSERT_EQ(depth.size(), 2U);

    EXPECT_EQ(depth.at(0).price, 101);
    EXPECT_EQ(depth.at(0).totalQuantity, 5U);
    EXPECT_EQ(depth.at(0).orderCount, 1U);

    EXPECT_EQ(depth.at(1).price, 100);
    EXPECT_EQ(depth.at(1).totalQuantity, 25U);
    EXPECT_EQ(depth.at(1).orderCount, 2U);
}

TEST_F(OrderBookSideSellTest, GetDepthForSellReturnsLowestToHighestPrices) {
    const auto ordersById = makeOrders();

    side.addOrder(104, 5);
    side.addOrder(104, 6);
    side.addOrder(103, 4);

    const std::vector<LevelSnapshot> depth = side.getDepth(2, ordersById);
    ASSERT_EQ(depth.size(), 2U);

    EXPECT_EQ(depth.at(0).price, 103);
    EXPECT_EQ(depth.at(0).totalQuantity, 7U);
    EXPECT_EQ(depth.at(0).orderCount, 1U);

    EXPECT_EQ(depth.at(1).price, 104);
    EXPECT_EQ(depth.at(1).totalQuantity, 20U);
    EXPECT_EQ(depth.at(1).orderCount, 2U);
}

TEST_F(OrderBookSideBuyTest, GetDepthRespectsRequestedLevelCount) {
    const auto ordersById = makeOrders();

    side.addOrder(99, 1);
    side.addOrder(100, 2);
    side.addOrder(101, 3);

    const std::vector<LevelSnapshot> depth = side.getDepth(1, ordersById);
    ASSERT_EQ(depth.size(), 1U);
    EXPECT_EQ(depth.at(0).price, 101);
}

TEST_F(OrderBookSideBuyTest, FindLevelReturnsNullptrForMissingPrice) {
    EXPECT_EQ(side.findLevel(12345), nullptr);
}

TEST_F(OrderBookSideBuyTest, RemoveEmptyLevelDeletesOnlyWhenQueueIsEmpty) {
    side.addOrder(100, 1);

    side.removeEmptyLevel(100);
    EXPECT_NE(side.findLevel(100), nullptr);

    auto* queue = side.findLevel(100);
    ASSERT_NE(queue, nullptr);
    queue->clear();

    side.removeEmptyLevel(100);
    EXPECT_EQ(side.findLevel(100), nullptr);
}

} // namespace
