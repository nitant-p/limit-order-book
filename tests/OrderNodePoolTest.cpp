#include "OrderBookSide.h"
#include "OrderNodePool.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <set>

namespace {

Order makeOrder(uint64_t id, int price = 100, int quantity = 10) {
    return Order{id, Side::BUY, Type::LIMIT, price, quantity};
}

} // namespace

TEST(OrderNodePoolTest, ConstructorInitializesCapacityAndCounts) {
    OrderNodePool pool(3);

    EXPECT_EQ(pool.capacity(), 3U);
    EXPECT_EQ(pool.activeCount(), 0U);
    EXPECT_EQ(pool.availableCount(), 3U);
    EXPECT_TRUE(pool.hasAvailable());
}

TEST(OrderNodePoolTest, AcquireReturnsInitializedNodeAndUpdatesCounts) {
    OrderNodePool pool(2);
    const Order order = makeOrder(1, 101, 25);

    OrderNode* node = pool.acquire(order);

    ASSERT_NE(node, nullptr);
    EXPECT_TRUE(pool.owns(node));
    EXPECT_EQ(node->order.id, order.id);
    EXPECT_EQ(node->order.side, order.side);
    EXPECT_EQ(node->order.type, order.type);
    EXPECT_EQ(node->order.price, order.price);
    EXPECT_EQ(node->order.quantity, order.quantity);
    EXPECT_EQ(node->prev, nullptr);
    EXPECT_EQ(node->next, nullptr);
    EXPECT_EQ(node->priceLevel, nullptr);
    EXPECT_LT(node->poolIndex, pool.capacity());
    EXPECT_EQ(pool.activeCount(), 1U);
    EXPECT_EQ(pool.availableCount(), 1U);
    EXPECT_TRUE(pool.hasAvailable());
}

TEST(OrderNodePoolTest, AcquireReturnsDistinctNodesUntilCapacityIsExhausted) {
    OrderNodePool pool(2);

    OrderNode* first = pool.acquire(makeOrder(1));
    OrderNode* second = pool.acquire(makeOrder(2));
    OrderNode* third = pool.acquire(makeOrder(3));

    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_NE(first, second);
    EXPECT_NE(first->poolIndex, second->poolIndex);
    EXPECT_EQ(third, nullptr);
    EXPECT_EQ(pool.activeCount(), 2U);
    EXPECT_EQ(pool.availableCount(), 0U);
    EXPECT_FALSE(pool.hasAvailable());
}

TEST(OrderNodePoolTest, ReleaseReturnsNodeToPoolAndClearsLinks) {
    OrderNodePool pool(1);
    OrderNode* node = pool.acquire(makeOrder(1));
    ASSERT_NE(node, nullptr);
    const std::size_t poolIndex = node->poolIndex;

    PriceLevel level;
    OrderNode previous{makeOrder(99), nullptr, node, &level, 999};
    OrderNode next{makeOrder(100), node, nullptr, &level, 1000};
    node->prev = &previous;
    node->next = &next;
    node->priceLevel = &level;

    EXPECT_TRUE(pool.release(node));

    EXPECT_EQ(node->prev, nullptr);
    EXPECT_EQ(node->next, nullptr);
    EXPECT_EQ(node->priceLevel, nullptr);
    EXPECT_EQ(node->poolIndex, poolIndex);
    EXPECT_EQ(pool.activeCount(), 0U);
    EXPECT_EQ(pool.availableCount(), 1U);
    EXPECT_TRUE(pool.hasAvailable());
}

TEST(OrderNodePoolTest, ReleaseRejectsNodeNotOwnedByPool) {
    OrderNodePool pool(1);
    OrderNode foreignNode{makeOrder(42), nullptr, nullptr, nullptr, 0};

    EXPECT_FALSE(pool.owns(&foreignNode));
    EXPECT_FALSE(pool.release(&foreignNode));
    EXPECT_EQ(pool.activeCount(), 0U);
    EXPECT_EQ(pool.availableCount(), 1U);
}

TEST(OrderNodePoolTest, ReleaseRejectsNullPointer) {
    OrderNodePool pool(1);

    EXPECT_FALSE(pool.release(nullptr));
    EXPECT_EQ(pool.activeCount(), 0U);
    EXPECT_EQ(pool.availableCount(), 1U);
}

TEST(OrderNodePoolTest, ReleaseRejectsDoubleRelease) {
    OrderNodePool pool(1);
    OrderNode* node = pool.acquire(makeOrder(1));
    ASSERT_NE(node, nullptr);

    EXPECT_TRUE(pool.release(node));
    EXPECT_FALSE(pool.release(node));
    EXPECT_EQ(pool.activeCount(), 0U);
    EXPECT_EQ(pool.availableCount(), 1U);
}

TEST(OrderNodePoolTest, ReleasedNodeCanBeReusedWithFreshOrderAndClearedLinks) {
    OrderNodePool pool(1);
    OrderNode* first = pool.acquire(makeOrder(1, 100, 10));
    ASSERT_NE(first, nullptr);
    const std::size_t poolIndex = first->poolIndex;

    PriceLevel level;
    first->prev = first;
    first->next = first;
    first->priceLevel = &level;

    ASSERT_TRUE(pool.release(first));

    const Order replacement = makeOrder(2, 105, 40);
    OrderNode* second = pool.acquire(replacement);

    ASSERT_EQ(second, first);
    EXPECT_EQ(second->order.id, replacement.id);
    EXPECT_EQ(second->order.price, replacement.price);
    EXPECT_EQ(second->order.quantity, replacement.quantity);
    EXPECT_EQ(second->prev, nullptr);
    EXPECT_EQ(second->next, nullptr);
    EXPECT_EQ(second->priceLevel, nullptr);
    EXPECT_EQ(second->poolIndex, poolIndex);
    EXPECT_EQ(pool.activeCount(), 1U);
    EXPECT_EQ(pool.availableCount(), 0U);
}

TEST(OrderNodePoolTest, OwnsReturnsTrueOnlyForNodesInsidePoolStorage) {
    OrderNodePool pool(2);
    OrderNode* first = pool.acquire(makeOrder(1));
    OrderNode* second = pool.acquire(makeOrder(2));
    OrderNode foreignNode{makeOrder(3), nullptr, nullptr, nullptr, 0};

    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_TRUE(pool.owns(first));
    EXPECT_TRUE(pool.owns(second));
    EXPECT_FALSE(pool.owns(&foreignNode));
    EXPECT_FALSE(pool.owns(nullptr));
}

TEST(OrderNodePoolTest, PoolIndexesAreStableAndUniqueAcrossAllSlots) {
    OrderNodePool pool(3);
    std::array<OrderNode*, 3> nodes{
        pool.acquire(makeOrder(1)),
        pool.acquire(makeOrder(2)),
        pool.acquire(makeOrder(3)),
    };

    std::set<std::size_t> indexes;
    for (OrderNode* node : nodes) {
        ASSERT_NE(node, nullptr);
        EXPECT_LT(node->poolIndex, pool.capacity());
        indexes.insert(node->poolIndex);
    }

    EXPECT_EQ(indexes.size(), nodes.size());

    for (OrderNode* node : nodes) {
        const std::size_t poolIndex = node->poolIndex;
        ASSERT_TRUE(pool.release(node));
        EXPECT_EQ(node->poolIndex, poolIndex);
    }
}
