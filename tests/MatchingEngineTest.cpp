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
    expectOrder(engine.getBuyOrders().front(), 3, Side::BUY, 101, 13);

    ASSERT_EQ(engine.getSellOrders().size(), 1U);
    expectOrder(engine.getSellOrders().front(), 1, Side::SELL, 100, 5);
}

TEST(MatchingEngineV1, SellSideMatchingRule_WorksSymmetrically) {
    MatchingEngine engine({}, {});

    engine.processOrder(Order{1, Side::BUY, 100, 8});
    engine.processOrder(Order{2, Side::SELL, 99, 3});

    EXPECT_TRUE(engine.getSellOrders().empty());
    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    expectOrder(engine.getBuyOrders().front(), 1, Side::BUY, 100, 5);
}

TEST(MatchingEngineV2, BuySelectsLowestCompatibleSell) {
    MatchingEngine engine({}, {});

    engine.processOrder(Order{1, Side::SELL, 103, 10});
    engine.processOrder(Order{2, Side::SELL, 101, 20});
    engine.processOrder(Order{3, Side::SELL, 102, 15});

    engine.processOrder(Order{4, Side::BUY, 102, 20});

    EXPECT_TRUE(engine.getBuyOrders().empty());
    ASSERT_EQ(engine.getSellOrders().size(), 2U);
    expectOrder(engine.getSellOrders().at(0), 1, Side::SELL, 103, 10);
    expectOrder(engine.getSellOrders().at(1), 3, Side::SELL, 102, 15);
}

TEST(MatchingEngineV2, SellSelectsHighestCompatibleBuy) {
    MatchingEngine engine({}, {});

    engine.processOrder(Order{1, Side::BUY, 97, 10});
    engine.processOrder(Order{2, Side::BUY, 99, 20});
    engine.processOrder(Order{3, Side::BUY, 98, 15});

    engine.processOrder(Order{4, Side::SELL, 98, 12});

    EXPECT_TRUE(engine.getSellOrders().empty());
    ASSERT_EQ(engine.getBuyOrders().size(), 3U);
    expectOrder(engine.getBuyOrders().at(0), 1, Side::BUY, 97, 10);
    expectOrder(engine.getBuyOrders().at(1), 2, Side::BUY, 99, 8);
    expectOrder(engine.getBuyOrders().at(2), 3, Side::BUY, 98, 15);
}

TEST(MatchingEngineV2, NoCompatibleSell_BuyRestsInBook) {
    MatchingEngine engine({}, {});

    engine.processOrder(Order{1, Side::SELL, 105, 7});
    engine.processOrder(Order{2, Side::SELL, 106, 9});
    engine.processOrder(Order{3, Side::BUY, 100, 4});

    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    expectOrder(engine.getBuyOrders().front(), 3, Side::BUY, 100, 4);
    ASSERT_EQ(engine.getSellOrders().size(), 2U);
}

TEST(MatchingEngineV2, EqualBestPriceUsesFirstSeenOrder) {
    MatchingEngine engine({}, {});

    engine.processOrder(Order{1, Side::SELL, 100, 5});
    engine.processOrder(Order{2, Side::SELL, 100, 7});
    engine.processOrder(Order{3, Side::BUY, 100, 6});

    ASSERT_EQ(engine.getBuyOrders().size(), 1U);
    expectOrder(engine.getBuyOrders().front(), 3, Side::BUY, 100, 1);
    ASSERT_EQ(engine.getSellOrders().size(), 1U);
    expectOrder(engine.getSellOrders().front(), 2, Side::SELL, 100, 7);
}
