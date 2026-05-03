#include "MatchingEngine.h"

#include <gtest/gtest.h>

namespace {

void expectOrder(const Order& order, int id, Side side, int price, int quantity) {
    EXPECT_EQ(order.id, id);
    EXPECT_EQ(order.side, side);
    EXPECT_EQ(order.price, price);
    EXPECT_EQ(order.quantity, quantity);
}

} // namespace

TEST(MatchingEngineV1, NoMatch_BuyOrderAddedToBook) {
    MatchingEngine engine({}, {});

    engine.processOrder(Order{1, Side::BUY, 100, 10});

    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    ASSERT_TRUE(engine.getSellOrders().empty());
    expectOrder(engine.getBuyOrders().front(), 1, Side::BUY, 100, 10);
}

TEST(MatchingEngineV1, ExactMatch_BothOrdersRemoved) {
    MatchingEngine engine({}, {});

    engine.processOrder(Order{1, Side::SELL, 100, 10});
    engine.processOrder(Order{2, Side::BUY, 100, 10});

    EXPECT_TRUE(engine.getBuyOrders().empty());
    EXPECT_TRUE(engine.getSellOrders().empty());
}

TEST(MatchingEngineV1, PartialFill_RestingOrderRemains) {
    MatchingEngine engine({}, {});

    engine.processOrder(Order{1, Side::SELL, 100, 20});
    engine.processOrder(Order{2, Side::BUY, 100, 5});

    EXPECT_TRUE(engine.getBuyOrders().empty());
    ASSERT_EQ(engine.getSellOrders().size(), 1U);
    expectOrder(engine.getSellOrders().front(), 1, Side::SELL, 100, 15);
}

TEST(MatchingEngineV1, IncomingLeftover_StoredInOwnBook) {
    MatchingEngine engine({}, {});

    engine.processOrder(Order{1, Side::SELL, 100, 5});
    engine.processOrder(Order{2, Side::BUY, 100, 20});

    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    EXPECT_TRUE(engine.getSellOrders().empty());
    expectOrder(engine.getBuyOrders().front(), 2, Side::BUY, 100, 15);
}

TEST(MatchingEngineV1, OnlyOneMatchPerIncomingOrder) {
    MatchingEngine engine({}, {});

    engine.processOrder(Order{1, Side::SELL, 100, 5});
    engine.processOrder(Order{2, Side::SELL, 99, 7});
    engine.processOrder(Order{3, Side::BUY, 101, 20});

    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    expectOrder(engine.getBuyOrders().front(), 3, Side::BUY, 101, 15);

    ASSERT_EQ(engine.getSellOrders().size(), 1U);
    expectOrder(engine.getSellOrders().front(), 2, Side::SELL, 99, 7);
}

TEST(MatchingEngineV1, SellSideMatchingRule_WorksSymmetrically) {
    MatchingEngine engine({}, {});

    engine.processOrder(Order{1, Side::BUY, 100, 8});
    engine.processOrder(Order{2, Side::SELL, 99, 3});

    EXPECT_TRUE(engine.getSellOrders().empty());
    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    expectOrder(engine.getBuyOrders().front(), 1, Side::BUY, 100, 5);
}
