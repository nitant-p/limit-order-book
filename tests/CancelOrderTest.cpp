#include "MatchingEngine.h"
#include "TestHelpers.h"

#include <gtest/gtest.h>

#include <vector>

namespace {

using test_helpers::levelIds;

class CancelOrderTest : public ::testing::Test {
protected:
    MatchingEngine engine;
};

} // namespace

TEST_F(CancelOrderTest, CancelMissingOrderReturnsFalseAndDoesNotMutateBook) {
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 4);  // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 103, 6); // id 2

    const auto beforeBuy100 = levelIds(engine.getBuyBook(), 100);
    const auto beforeSell103 = levelIds(engine.getSellOrders(), 103);

    const bool ok = engine.cancelOrder(999);

    EXPECT_FALSE(ok);
    EXPECT_EQ(levelIds(engine.getBuyBook(), 100), beforeBuy100);
    EXPECT_EQ(levelIds(engine.getSellOrders(), 103), beforeSell103);
}

TEST_F(CancelOrderTest, CancelExistingBuyOrderRemovesLastOrderAtPriceLevel) {
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 10); // id 1

    const bool ok = engine.cancelOrder(1);

    EXPECT_TRUE(ok);
    EXPECT_TRUE(engine.getBuyBook().empty());
}

TEST_F(CancelOrderTest, CancelExistingSellOrderRemovesOnlyTargetFromPriceLevel) {
    engine.processOrder(Side::SELL, Type::LIMIT, 101, 5); // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 101, 7); // id 2

    const bool ok = engine.cancelOrder(1);

    EXPECT_TRUE(ok);
    EXPECT_EQ(levelIds(engine.getSellOrders(), 101), std::vector<uint64_t>({2}));
}

TEST_F(CancelOrderTest, CancelMiddleOrderPreservesRemainingFifoOrder) {
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 2); // id 1
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 3); // id 2
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 4); // id 3

    const bool ok = engine.cancelOrder(2);

    EXPECT_TRUE(ok);
    EXPECT_EQ(levelIds(engine.getBuyBook(), 100), std::vector<uint64_t>({1, 3}));
}

TEST_F(CancelOrderTest, CancelTwiceSecondCallReturnsFalseWithNoFurtherMutation) {
    engine.processOrder(Side::BUY, Type::LIMIT, 101, 3); // id 1
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 2); // id 2

    const bool first = engine.cancelOrder(1);
    const auto afterFirstBuy100 = levelIds(engine.getBuyBook(), 100);

    const bool second = engine.cancelOrder(1);

    EXPECT_TRUE(first);
    EXPECT_FALSE(second);
    EXPECT_EQ(levelIds(engine.getBuyBook(), 100), afterFirstBuy100);
}

TEST_F(CancelOrderTest, CancelPartiallyFilledRestingOrderRemovesItsRemainder) {
    engine.processOrder(Side::SELL, Type::LIMIT, 100, 10); // id 1
    engine.processOrder(Side::BUY, Type::LIMIT, 100, 4);   // id 2

    ASSERT_EQ(levelIds(engine.getSellOrders(), 100), std::vector<uint64_t>({1}));

    const bool ok = engine.cancelOrder(1);

    EXPECT_TRUE(ok);
    EXPECT_TRUE(engine.getSellOrders().empty());
}

TEST_F(CancelOrderTest, CancelDoesNotAffectOppositeSideBook) {
    engine.processOrder(Side::BUY, Type::LIMIT, 99, 2);   // id 1
    engine.processOrder(Side::SELL, Type::LIMIT, 105, 7); // id 2
    engine.processOrder(Side::SELL, Type::LIMIT, 106, 8); // id 3

    const auto beforeSell105 = levelIds(engine.getSellOrders(), 105);
    const auto beforeSell106 = levelIds(engine.getSellOrders(), 106);

    const bool ok = engine.cancelOrder(1);

    EXPECT_TRUE(ok);
    EXPECT_EQ(levelIds(engine.getSellOrders(), 105), beforeSell105);
    EXPECT_EQ(levelIds(engine.getSellOrders(), 106), beforeSell106);
    EXPECT_TRUE(engine.getBuyBook().empty());
}
