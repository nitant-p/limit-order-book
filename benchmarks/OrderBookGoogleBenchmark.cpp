#include "MatchingEngine.h"
#include "OrderBookSide.h"

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

namespace {

constexpr std::int64_t kTinyBatch = 1'000;
constexpr std::int64_t kSmallBatch = 10'000;
constexpr std::int64_t kMediumBatch = 100'000;
constexpr std::int64_t kLargeBatch = 1'000'000;
constexpr int kPassiveBuyBase = 90;
constexpr int kPassiveSellBase = 110;

volatile std::uint64_t depthReadSink = 0;

// Counts all resting orders left across both sides after a benchmark iteration.
std::size_t activeOrderCount(const MatchingEngine& engine) {
    return engine.getBuyBook().orderCount() + engine.getSellOrders().orderCount();
}

// Generates small deterministic quantities without involving RNG inside timed loops.
int quantityFor(std::int64_t i) {
    return 1 + static_cast<int>(i % 10);
}

// Returns non-crossing buy prices so add-only workloads build passive liquidity.
int passiveBuyPrice(std::int64_t i) {
    return kPassiveBuyBase + static_cast<int>(i % 10);
}

// Returns non-crossing sell prices so add-only workloads build passive liquidity.
int passiveSellPrice(std::int64_t i) {
    return kPassiveSellBase + static_cast<int>(i % 10);
}

// Adds common Google Benchmark counters for domain-level sanity checks.
void setCommonCounters(
    benchmark::State& state,
    std::int64_t operationsPerIteration,
    std::uint64_t tradesGenerated,
    std::size_t finalActiveOrders
) {
    state.SetItemsProcessed(state.iterations() * operationsPerIteration);
    state.counters["trades_generated"] = benchmark::Counter(
        static_cast<double>(tradesGenerated),
        benchmark::Counter::kAvgIterations
    );
    state.counters["final_active_orders"] = static_cast<double>(finalActiveOrders);
}

// Seeds sell-side liquidity before timing market-buy matching.
void seedPassiveSellLiquidity(MatchingEngine& engine, std::int64_t orders) {
    for (std::int64_t i = 0; i < orders; ++i) {
        engine.processOrder(Side::SELL, Type::LIMIT, 100 + static_cast<int>(i % 50), 1);
    }
}

// Seeds buy-side orders with known IDs before timing cancel/modify APIs.
void seedPassiveBuyOrders(MatchingEngine& engine, std::int64_t orders, int price, int quantity) {
    for (std::int64_t i = 0; i < orders; ++i) {
        engine.processOrder(Side::BUY, Type::LIMIT, price, quantity);
    }
}

// Seeds one buy-side price level directly for OrderBookSide API benchmarks.
void seedSideBuyOrders(OrderBookSide& side, std::int64_t orders, int price, int quantity) {
    for (std::int64_t i = 0; i < orders; ++i) {
        side.addOrder(static_cast<std::uint64_t>(i + 1), Side::BUY, Type::LIMIT, price, quantity);
    }
}

// Seeds many buy-side price levels directly for getDepth and best-price reads.
void seedSideDepth(OrderBookSide& side, std::int64_t levels) {
    for (std::int64_t i = 0; i < levels; ++i) {
        side.addOrder(
            static_cast<std::uint64_t>(i + 1),
            Side::BUY,
            Type::LIMIT,
            100'000 - static_cast<int>(i),
            quantityFor(i)
        );
    }
}

// Measures processOrder for non-crossing LIMIT orders that only rest on the book.
void BM_ProcessOrder_AddOnly(benchmark::State& state) {
    const std::int64_t orders = state.range(0);
    std::uint64_t tradesGenerated = 0;
    std::size_t finalActiveOrders = 0;

    for (auto _ : state) {
        state.PauseTiming();
        MatchingEngine engine;
        std::uint64_t iterationTrades = 0;
        state.ResumeTiming();

        for (std::int64_t i = 0; i < orders; ++i) {
            const bool buySide = (i % 2) == 0;
            const std::vector<Trade> trades = engine.processOrder(
                buySide ? Side::BUY : Side::SELL,
                Type::LIMIT,
                buySide ? passiveBuyPrice(i) : passiveSellPrice(i),
                quantityFor(i)
            );
            iterationTrades += trades.size();
            benchmark::DoNotOptimize(trades.size());
        }

        benchmark::ClobberMemory();
        state.PauseTiming();
        tradesGenerated += iterationTrades;
        finalActiveOrders = activeOrderCount(engine);
        state.ResumeTiming();
    }

    setCommonCounters(state, orders, tradesGenerated, finalActiveOrders);
}

// Measures processOrder when every incoming MARKET buy consumes one resting sell.
void BM_ProcessOrder_MatchHeavy(benchmark::State& state) {
    const std::int64_t orders = state.range(0);
    std::uint64_t tradesGenerated = 0;
    std::size_t finalActiveOrders = 0;

    for (auto _ : state) {
        state.PauseTiming();
        MatchingEngine engine;
        seedPassiveSellLiquidity(engine, orders);
        std::uint64_t iterationTrades = 0;
        state.ResumeTiming();

        for (std::int64_t i = 0; i < orders; ++i) {
            const std::vector<Trade> trades = engine.processOrder(Side::BUY, Type::MARKET, 0, 1);
            iterationTrades += trades.size();
            benchmark::DoNotOptimize(trades.size());
        }

        benchmark::ClobberMemory();
        state.PauseTiming();
        tradesGenerated += iterationTrades;
        finalActiveOrders = activeOrderCount(engine);
        state.ResumeTiming();
    }

    setCommonCounters(state, orders, tradesGenerated, finalActiveOrders);
}

// Measures processOrder over a deterministic blend of passive limits and markets.
void BM_ProcessOrder_Mixed(benchmark::State& state) {
    const std::int64_t orders = state.range(0);
    std::uint64_t tradesGenerated = 0;
    std::size_t finalActiveOrders = 0;

    for (auto _ : state) {
        state.PauseTiming();
        MatchingEngine engine;
        std::uint64_t iterationTrades = 0;
        state.ResumeTiming();

        for (std::int64_t i = 0; i < orders; ++i) {
            std::vector<Trade> trades;
            switch (i % 6) {
                case 0:
                case 1:
                    trades = engine.processOrder(Side::BUY, Type::LIMIT, passiveBuyPrice(i), quantityFor(i));
                    break;
                case 2:
                case 3:
                    trades = engine.processOrder(Side::SELL, Type::LIMIT, passiveSellPrice(i), quantityFor(i));
                    break;
                case 4:
                    trades = engine.processOrder(Side::BUY, Type::MARKET, 0, quantityFor(i));
                    break;
                default:
                    trades = engine.processOrder(Side::SELL, Type::MARKET, 0, quantityFor(i));
                    break;
            }

            iterationTrades += trades.size();
            benchmark::DoNotOptimize(trades.size());
        }

        benchmark::ClobberMemory();
        state.PauseTiming();
        tradesGenerated += iterationTrades;
        finalActiveOrders = activeOrderCount(engine);
        state.ResumeTiming();
    }

    setCommonCounters(state, orders, tradesGenerated, finalActiveOrders);
}

// Measures cancelOrder for existing IDs after setup creates a known ID range.
void BM_CancelOrder_Existing(benchmark::State& state) {
    const std::int64_t orders = state.range(0);
    std::size_t finalActiveOrders = 0;
    std::uint64_t successfulCancels = 0;

    for (auto _ : state) {
        state.PauseTiming();
        MatchingEngine engine;
        seedPassiveBuyOrders(engine, orders, 100, 10);
        std::uint64_t iterationCancels = 0;
        state.ResumeTiming();

        for (std::int64_t i = 0; i < orders; ++i) {
            const bool ok = engine.cancelOrder(static_cast<std::uint64_t>(i + 1));
            iterationCancels += ok ? 1 : 0;
            int okValue = ok ? 1 : 0;
            benchmark::DoNotOptimize(okValue);
        }

        benchmark::ClobberMemory();
        state.PauseTiming();
        successfulCancels += iterationCancels;
        finalActiveOrders = activeOrderCount(engine);
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations() * orders);
    state.counters["successful_cancels"] = benchmark::Counter(
        static_cast<double>(successfulCancels),
        benchmark::Counter::kAvgIterations
    );
    state.counters["final_active_orders"] = static_cast<double>(finalActiveOrders);
}

// Measures modifyOrder when reducing quantity at the same price, preserving queue position.
void BM_ModifyOrder_SamePriceReduce(benchmark::State& state) {
    const std::int64_t orders = state.range(0);
    std::size_t finalActiveOrders = 0;
    std::uint64_t successfulModifies = 0;

    for (auto _ : state) {
        state.PauseTiming();
        MatchingEngine engine;
        seedPassiveBuyOrders(engine, orders, 100, 10);
        std::uint64_t iterationModifies = 0;
        state.ResumeTiming();

        for (std::int64_t i = 0; i < orders; ++i) {
            const bool ok = engine.modifyOrder(static_cast<std::uint64_t>(i + 1), 100, 5);
            iterationModifies += ok ? 1 : 0;
            int okValue = ok ? 1 : 0;
            benchmark::DoNotOptimize(okValue);
        }

        benchmark::ClobberMemory();
        state.PauseTiming();
        successfulModifies += iterationModifies;
        finalActiveOrders = activeOrderCount(engine);
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations() * orders);
    state.counters["successful_modifies"] = benchmark::Counter(
        static_cast<double>(successfulModifies),
        benchmark::Counter::kAvgIterations
    );
    state.counters["final_active_orders"] = static_cast<double>(finalActiveOrders);
}

// Measures modifyOrder when the price changes and the order moves to another level.
void BM_ModifyOrder_PriceChange(benchmark::State& state) {
    const std::int64_t orders = state.range(0);
    std::size_t finalActiveOrders = 0;
    std::uint64_t successfulModifies = 0;

    for (auto _ : state) {
        state.PauseTiming();
        MatchingEngine engine;
        seedPassiveBuyOrders(engine, orders, 100, 10);
        std::uint64_t iterationModifies = 0;
        state.ResumeTiming();

        for (std::int64_t i = 0; i < orders; ++i) {
            const int newPrice = 101 + static_cast<int>(i % 100);
            const bool ok = engine.modifyOrder(static_cast<std::uint64_t>(i + 1), newPrice, 10);
            iterationModifies += ok ? 1 : 0;
            int okValue = ok ? 1 : 0;
            benchmark::DoNotOptimize(okValue);
        }

        benchmark::ClobberMemory();
        state.PauseTiming();
        successfulModifies += iterationModifies;
        finalActiveOrders = activeOrderCount(engine);
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations() * orders);
    state.counters["successful_modifies"] = benchmark::Counter(
        static_cast<double>(successfulModifies),
        benchmark::Counter::kAvgIterations
    );
    state.counters["final_active_orders"] = static_cast<double>(finalActiveOrders);
}

// Measures OrderBookSide::addOrder on one side without MatchingEngine orchestration.
void BM_OrderBookSide_AddOrder(benchmark::State& state) {
    const std::int64_t orders = state.range(0);
    std::size_t finalOrderCount = 0;

    for (auto _ : state) {
        state.PauseTiming();
        OrderBookSide side{Side::BUY};
        state.ResumeTiming();

        for (std::int64_t i = 0; i < orders; ++i) {
            side.addOrder(
                static_cast<std::uint64_t>(i + 1),
                Side::BUY,
                Type::LIMIT,
                passiveBuyPrice(i),
                quantityFor(i)
            );
        }

        benchmark::ClobberMemory();
        state.PauseTiming();
        finalOrderCount = side.orderCount();
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations() * orders);
    state.counters["final_order_count"] = static_cast<double>(finalOrderCount);
}

// Measures OrderBookSide::bestPrice repeated against a populated side.
void BM_OrderBookSide_BestPrice(benchmark::State& state) {
    const std::int64_t reads = state.range(0);

    for (auto _ : state) {
        state.PauseTiming();
        OrderBookSide side{Side::BUY};
        seedSideDepth(side, 1'000);
        state.ResumeTiming();

        for (std::int64_t i = 0; i < reads; ++i) {
            std::optional<int> best = side.bestPrice();
            benchmark::DoNotOptimize(best);
        }
    }

    state.SetItemsProcessed(state.iterations() * reads);
}

// Measures OrderBookSide::getBestOrder repeated against a populated side.
void BM_OrderBookSide_GetBestOrder(benchmark::State& state) {
    const std::int64_t reads = state.range(0);

    for (auto _ : state) {
        state.PauseTiming();
        OrderBookSide side{Side::BUY};
        seedSideDepth(side, 1'000);
        state.ResumeTiming();

        for (std::int64_t i = 0; i < reads; ++i) {
            const Order* order = side.getBestOrder();
            benchmark::DoNotOptimize(order);
        }
    }

    state.SetItemsProcessed(state.iterations() * reads);
}

// Measures OrderBookSide::findOrder for existing IDs.
void BM_OrderBookSide_FindOrder(benchmark::State& state) {
    const std::int64_t reads = state.range(0);

    for (auto _ : state) {
        state.PauseTiming();
        OrderBookSide side{Side::BUY};
        seedSideBuyOrders(side, reads, 100, 10);
        state.ResumeTiming();

        for (std::int64_t i = 0; i < reads; ++i) {
            const Order* order = side.findOrder(static_cast<std::uint64_t>(i + 1));
            benchmark::DoNotOptimize(order);
        }
    }

    state.SetItemsProcessed(state.iterations() * reads);
}

// Measures OrderBookSide::deleteOrderById for existing IDs.
void BM_OrderBookSide_DeleteOrderById(benchmark::State& state) {
    const std::int64_t orders = state.range(0);
    std::size_t finalOrderCount = 0;
    std::uint64_t successfulDeletes = 0;

    for (auto _ : state) {
        state.PauseTiming();
        OrderBookSide side{Side::BUY};
        seedSideBuyOrders(side, orders, 100, 10);
        std::uint64_t iterationDeletes = 0;
        state.ResumeTiming();

        for (std::int64_t i = 0; i < orders; ++i) {
            const bool ok = side.deleteOrderById(static_cast<std::uint64_t>(i + 1));
            iterationDeletes += ok ? 1 : 0;
            int okValue = ok ? 1 : 0;
            benchmark::DoNotOptimize(okValue);
        }

        benchmark::ClobberMemory();
        state.PauseTiming();
        successfulDeletes += iterationDeletes;
        finalOrderCount = side.orderCount();
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations() * orders);
    state.counters["successful_deletes"] = benchmark::Counter(
        static_cast<double>(successfulDeletes),
        benchmark::Counter::kAvgIterations
    );
    state.counters["final_order_count"] = static_cast<double>(finalOrderCount);
}

// Measures OrderBookSide::reduceOrderQuantity when each order remains resting.
void BM_OrderBookSide_ReduceQuantityPartial(benchmark::State& state) {
    const std::int64_t orders = state.range(0);
    std::size_t finalOrderCount = 0;
    std::uint64_t successfulReductions = 0;

    for (auto _ : state) {
        state.PauseTiming();
        OrderBookSide side{Side::BUY};
        seedSideBuyOrders(side, orders, 100, 10);
        std::uint64_t iterationReductions = 0;
        state.ResumeTiming();

        for (std::int64_t i = 0; i < orders; ++i) {
            const bool ok = side.reduceOrderQuantity(static_cast<std::uint64_t>(i + 1), 1);
            iterationReductions += ok ? 1 : 0;
            int okValue = ok ? 1 : 0;
            benchmark::DoNotOptimize(okValue);
        }

        benchmark::ClobberMemory();
        state.PauseTiming();
        successfulReductions += iterationReductions;
        finalOrderCount = side.orderCount();
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations() * orders);
    state.counters["successful_reductions"] = benchmark::Counter(
        static_cast<double>(successfulReductions),
        benchmark::Counter::kAvgIterations
    );
    state.counters["final_order_count"] = static_cast<double>(finalOrderCount);
}

// Measures OrderBookSide::reduceOrderQuantity when each reduction fully removes an order.
void BM_OrderBookSide_ReduceQuantityExactFill(benchmark::State& state) {
    const std::int64_t orders = state.range(0);
    std::size_t finalOrderCount = 0;
    std::uint64_t successfulReductions = 0;

    for (auto _ : state) {
        state.PauseTiming();
        OrderBookSide side{Side::BUY};
        seedSideBuyOrders(side, orders, 100, 1);
        std::uint64_t iterationReductions = 0;
        state.ResumeTiming();

        for (std::int64_t i = 0; i < orders; ++i) {
            const bool ok = side.reduceOrderQuantity(static_cast<std::uint64_t>(i + 1), 1);
            iterationReductions += ok ? 1 : 0;
            int okValue = ok ? 1 : 0;
            benchmark::DoNotOptimize(okValue);
        }

        benchmark::ClobberMemory();
        state.PauseTiming();
        successfulReductions += iterationReductions;
        finalOrderCount = side.orderCount();
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations() * orders);
    state.counters["successful_reductions"] = benchmark::Counter(
        static_cast<double>(successfulReductions),
        benchmark::Counter::kAvgIterations
    );
    state.counters["final_order_count"] = static_cast<double>(finalOrderCount);
}

// Measures OrderBookSide::modifyOrder for same-price quantity reductions.
void BM_OrderBookSide_ModifySamePriceReduce(benchmark::State& state) {
    const std::int64_t orders = state.range(0);
    std::size_t finalOrderCount = 0;
    std::uint64_t successfulModifies = 0;

    for (auto _ : state) {
        state.PauseTiming();
        OrderBookSide side{Side::BUY};
        seedSideBuyOrders(side, orders, 100, 10);
        std::uint64_t iterationModifies = 0;
        state.ResumeTiming();

        for (std::int64_t i = 0; i < orders; ++i) {
            const Order updatedOrder{
                static_cast<std::uint64_t>(i + 1),
                Side::BUY,
                Type::LIMIT,
                100,
                5
            };
            const bool ok = side.modifyOrder(updatedOrder, false);
            iterationModifies += ok ? 1 : 0;
            int okValue = ok ? 1 : 0;
            benchmark::DoNotOptimize(okValue);
        }

        benchmark::ClobberMemory();
        state.PauseTiming();
        successfulModifies += iterationModifies;
        finalOrderCount = side.orderCount();
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations() * orders);
    state.counters["successful_modifies"] = benchmark::Counter(
        static_cast<double>(successfulModifies),
        benchmark::Counter::kAvgIterations
    );
    state.counters["final_order_count"] = static_cast<double>(finalOrderCount);
}

// Measures OrderBookSide::modifyOrder when each order changes price level.
void BM_OrderBookSide_ModifyPriceChange(benchmark::State& state) {
    const std::int64_t orders = state.range(0);
    std::size_t finalOrderCount = 0;
    std::uint64_t successfulModifies = 0;

    for (auto _ : state) {
        state.PauseTiming();
        OrderBookSide side{Side::BUY};
        seedSideBuyOrders(side, orders, 100, 10);
        std::uint64_t iterationModifies = 0;
        state.ResumeTiming();

        for (std::int64_t i = 0; i < orders; ++i) {
            const Order updatedOrder{
                static_cast<std::uint64_t>(i + 1),
                Side::BUY,
                Type::LIMIT,
                101 + static_cast<int>(i % 100),
                10
            };
            const bool ok = side.modifyOrder(updatedOrder, true);
            iterationModifies += ok ? 1 : 0;
            int okValue = ok ? 1 : 0;
            benchmark::DoNotOptimize(okValue);
        }

        benchmark::ClobberMemory();
        state.PauseTiming();
        successfulModifies += iterationModifies;
        finalOrderCount = side.orderCount();
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations() * orders);
    state.counters["successful_modifies"] = benchmark::Counter(
        static_cast<double>(successfulModifies),
        benchmark::Counter::kAvgIterations
    );
    state.counters["final_order_count"] = static_cast<double>(finalOrderCount);
}

// Measures OrderBookSide::getDepth for shallow, medium, and deep snapshots.
void BM_OrderBookSide_GetDepth(benchmark::State& state) {
    const std::int64_t depthLevels = state.range(0);
    const std::int64_t reads = state.range(1);
    const std::int64_t seededLevels = std::max<std::int64_t>(depthLevels * 2, 1'000);
    std::size_t finalOrderCount = 0;
    std::uint64_t checksum = 0;

    for (auto _ : state) {
        state.PauseTiming();
        OrderBookSide side{Side::BUY};
        seedSideDepth(side, seededLevels);
        std::uint64_t iterationChecksum = 0;
        state.ResumeTiming();

        for (std::int64_t i = 0; i < reads; ++i) {
            const std::vector<LevelSnapshot> depth = side.getDepth(static_cast<std::size_t>(depthLevels));
            iterationChecksum += static_cast<std::uint64_t>(depth.size());
            if (!depth.empty()) {
                iterationChecksum += static_cast<std::uint64_t>(depth.front().price);
                iterationChecksum += depth.front().totalQuantity;
                iterationChecksum += static_cast<std::uint64_t>(depth.back().orderCount);
            }
            benchmark::DoNotOptimize(depth.size());
        }

        benchmark::ClobberMemory();
        state.PauseTiming();
        checksum += iterationChecksum;
        finalOrderCount = side.orderCount();
        state.ResumeTiming();
    }

    depthReadSink = checksum;
    state.SetItemsProcessed(state.iterations() * reads);
    state.counters["depth_levels"] = static_cast<double>(depthLevels);
    state.counters["final_order_count"] = static_cast<double>(finalOrderCount);
}

} // namespace

// Arg(value) supplies state.range(0), which this file treats as operations per iteration.
BENCHMARK(BM_ProcessOrder_AddOnly)
    ->Arg(kTinyBatch)
    ->Arg(kSmallBatch)
    ->Arg(kMediumBatch)
    ->Arg(kLargeBatch)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_ProcessOrder_MatchHeavy)
    ->Arg(kTinyBatch)
    ->Arg(kSmallBatch)
    ->Arg(kMediumBatch)
    ->Arg(kLargeBatch)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_ProcessOrder_Mixed)
    ->Arg(kTinyBatch)
    ->Arg(kSmallBatch)
    ->Arg(kMediumBatch)
    ->Arg(kLargeBatch)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_CancelOrder_Existing)
    ->Arg(kTinyBatch)
    ->Arg(kSmallBatch)
    ->Arg(kMediumBatch)
    ->Arg(kLargeBatch)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_ModifyOrder_SamePriceReduce)
    ->Arg(kTinyBatch)
    ->Arg(kSmallBatch)
    ->Arg(kMediumBatch)
    ->Arg(kLargeBatch)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_ModifyOrder_PriceChange)
    ->Arg(kTinyBatch)
    ->Arg(kSmallBatch)
    ->Arg(kMediumBatch)
    ->Arg(kLargeBatch)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_OrderBookSide_AddOrder)
    ->Arg(kTinyBatch)
    ->Arg(kSmallBatch)
    ->Arg(kMediumBatch)
    ->Arg(kLargeBatch)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_OrderBookSide_BestPrice)
    ->Arg(kTinyBatch)
    ->Arg(kSmallBatch)
    ->Arg(kMediumBatch)
    ->Arg(kLargeBatch)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_OrderBookSide_GetBestOrder)
    ->Arg(kTinyBatch)
    ->Arg(kSmallBatch)
    ->Arg(kMediumBatch)
    ->Arg(kLargeBatch)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_OrderBookSide_FindOrder)
    ->Arg(kTinyBatch)
    ->Arg(kSmallBatch)
    ->Arg(kMediumBatch)
    ->Arg(kLargeBatch)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_OrderBookSide_DeleteOrderById)
    ->Arg(kTinyBatch)
    ->Arg(kSmallBatch)
    ->Arg(kMediumBatch)
    ->Arg(kLargeBatch)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_OrderBookSide_ReduceQuantityPartial)
    ->Arg(kTinyBatch)
    ->Arg(kSmallBatch)
    ->Arg(kMediumBatch)
    ->Arg(kLargeBatch)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_OrderBookSide_ReduceQuantityExactFill)
    ->Arg(kTinyBatch)
    ->Arg(kSmallBatch)
    ->Arg(kMediumBatch)
    ->Arg(kLargeBatch)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_OrderBookSide_ModifySamePriceReduce)
    ->Arg(kTinyBatch)
    ->Arg(kSmallBatch)
    ->Arg(kMediumBatch)
    ->Arg(kLargeBatch)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_OrderBookSide_ModifyPriceChange)
    ->Arg(kTinyBatch)
    ->Arg(kSmallBatch)
    ->Arg(kMediumBatch)
    ->Arg(kLargeBatch)
    ->Unit(benchmark::kNanosecond);

// Args({depth, reads}) supplies state.range(0)=depth levels and state.range(1)=read count.
BENCHMARK(BM_OrderBookSide_GetDepth)
    ->Args({10, kTinyBatch})
    ->Args({100, kTinyBatch})
    ->Args({1000, kTinyBatch})
    ->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
