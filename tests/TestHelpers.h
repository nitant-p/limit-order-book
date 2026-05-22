#pragma once

#include "OrderBookSide.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace test_helpers {

inline std::vector<uint64_t> levelIds(const OrderBookSide& side, int price) {
    std::vector<uint64_t> ids;
    const PriceLevel* level = side.findLevel(price);
    if (level == nullptr) return ids;

    for (OrderNode* curr = level->head; curr != nullptr; curr = curr->next) {
        ids.push_back(curr->order.id);
    }

    return ids;
}

inline void expectLevelIntegrity(
    const OrderBookSide& side,
    int price,
    const std::vector<uint64_t>& expectedIds,
    uint64_t expectedTotalQty
) {
    const PriceLevel* level = side.findLevel(price);
    ASSERT_NE(level, nullptr);

    if (expectedIds.empty()) {
        EXPECT_EQ(level->head, nullptr);
        EXPECT_EQ(level->end, nullptr);
        EXPECT_EQ(level->orderCount, 0U);
        EXPECT_EQ(level->totalQuantity, 0U);
        return;
    }

    ASSERT_NE(level->head, nullptr);
    ASSERT_NE(level->end, nullptr);

    std::vector<uint64_t> observedIds;
    uint64_t observedQty = 0;
    OrderNode* prev = nullptr;

    for (OrderNode* curr = level->head; curr != nullptr; curr = curr->next) {
        EXPECT_EQ(curr->prev, prev);
        if (prev != nullptr) {
            EXPECT_EQ(prev->next, curr);
        }

        observedIds.push_back(curr->order.id);
        observedQty += static_cast<uint64_t>(curr->order.quantity);
        prev = curr;
    }

    EXPECT_EQ(prev, level->end);

    std::vector<uint64_t> reverseIds;
    OrderNode* next = nullptr;
    for (OrderNode* curr = level->end; curr != nullptr; curr = curr->prev) {
        EXPECT_EQ(curr->next, next);
        reverseIds.push_back(curr->order.id);
        next = curr;
    }
    EXPECT_EQ(next, level->head);

    std::vector<uint64_t> expectedReverse(expectedIds.rbegin(), expectedIds.rend());
    EXPECT_EQ(observedIds, expectedIds);
    EXPECT_EQ(reverseIds, expectedReverse);
    EXPECT_EQ(level->orderCount, expectedIds.size());
    EXPECT_EQ(level->totalQuantity, expectedTotalQty);
}

} // namespace test_helpers
