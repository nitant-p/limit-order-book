# Limit Order Book (C++)
[![CI](https://github.com/nitant-p/order-book/actions/workflows/ci.yml/badge.svg)](https://github.com/nitant-p/order-book/actions/workflows/ci.yml)
[![Coverage](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2Fnitant-p%2Forder-book%2Fmain%2F.github%2Fbadges%2Fcoverage.json)](https://github.com/nitant-p/order-book/actions/workflows/coverage.yml)

High-performance C++ matching engine and order book implementation for exchange-style trade simulation.

## Overview
This project implements a central limit order book (CLOB) with deterministic matching behavior, order lifecycle operations, and automated validation through unit tests and CI.

## Core Features
- Supports `BUY` and `SELL` sides.
- Supports `LIMIT` and `MARKET` order types.
- Enforces price-time priority within price levels (FIFO queue per level).
- Multi-level matching across the book until fill or stop condition.
- Best-price execution:
- Incoming buys match lowest available asks first.
- Incoming sells match highest available bids first.
- Partial fill handling for both incoming and resting orders.
- Automatic cleanup of empty price levels after fills/cancels.
- Engine-managed monotonic `uint64_t` order IDs.
- Order lifecycle API includes `processOrder(side, type, price, quantity)`.
- Order lifecycle API includes `cancelOrder(orderId)`.
- Order lifecycle API includes `modifyOrder(orderId, newPrice, newQuantity)`.
- Trade capture per processed order (returns `std::vector<Trade>`).

## Architecture
- Matching engine: [`MatchingEngine.h`](./include/MatchingEngine.h), [`MatchingEngine.cpp`](./src/MatchingEngine.cpp)
- Order model: [`Order.h`](./include/Order.h)
- Trade model: [`Trade.h`](./include/Trade.h)
- Test suite: `tests/` (category-based fixtures)

The engine owns two independent order books, one for bids and one for asks. The engine handles order processing, matching, cancellation, modification, trade capture, and ID-side routing; each `OrderBookSide` owns its own price levels and order nodes.

![Matching engine architecture](./docs/architecture/matching-engine-architecture.png)

Each order book stores price levels in `std::map<int, PriceLevel>`. Conceptually this is an ordered binary tree keyed by price. A `PriceLevel` stores aggregate level metadata and points to the head and tail of its FIFO order-node queue.

![Price level tree](./docs/architecture/price-level-tree.png)

Each order book also stores active orders by ID in `std::unordered_map<uint64_t, std::unique_ptr<OrderNode>>`. The map owns the nodes. The nodes link to each other as a doubly linked list, and each node points back to its `PriceLevel`.

![Order linked list](./docs/architecture/order-linked-list.png)

## Build
```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
```

Run executable:
```bash
./build/order_book
```

## Test
```bash
ctest --test-dir build --output-on-failure
```

## Test Categories
The suite is organized by behavior category:

- `BestMatchingBySideTest`: best-price matching and price-time behavior for buy/sell matching loops.
- `OrderTypeBehaviorTest`: limit vs market semantics and liquidity edge cases.
- `CancelOrderTest`: cancel API behavior, queue cleanup, and non-mutation guarantees.
- `ModifyOrderTest`: modify API behavior, priority impacts, and invalid input handling.
- `OrderIdBehaviorTest`: engine-generated monotonic/unique order ID behavior.

Run a single category:
```bash
ctest --test-dir build -R ModifyOrderTest --output-on-failure
```

## V1 Benchmark Results
This section is the V1 benchmark baseline. Charts are generated from [`docs/benchmark_results_v1.csv`](./docs/benchmark_results_v1.csv) using a dependency-free Python SVG generator:

```bash
python3 scripts/generate_benchmark_graphs.py
```

Build and run the Google Benchmark executable:

```bash
cmake -S . -B build -DBUILD_TESTING=ON -DBUILD_GOOGLE_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --target order_book_google_benchmark --parallel
./build/order_book_google_benchmark --benchmark_repetitions=3 --benchmark_report_aggregates_only=true
```

The benchmark build disables hot-path console logging with `ORDER_BOOK_DISABLE_LOGGING`, so `processOrder` and related timed paths are not dominated by `std::cout`.

### MatchingEngine APIs
![MatchingEngine API latency](./docs/benchmark_graphs/engine_latency.svg)

![MatchingEngine API throughput](./docs/benchmark_graphs/engine_throughput.svg)

Findings:
- `processOrder` add-only still pays for node allocation and price-level map updates, but order-ID indexing is now average O(1).
- Match-heavy `processOrder` remains competitive because market orders remove resting liquidity instead of growing the book.
- Mixed flow performs best among the engine order-processing cases because it combines passive adds with liquidity removal.
- `cancelOrder` benefits from average O(1) side-level order lookup before unlinking the node.
- Same-price `modifyOrder` is faster than price-change modify because it preserves queue position and avoids relinking across price levels.

### OrderBookSide APIs
![OrderBookSide API latency](./docs/benchmark_graphs/side_latency_10k.svg)

![OrderBookSide API throughput](./docs/benchmark_graphs/side_throughput_10k.svg)

Findings:
- Direct `OrderBookSide` calls are faster than equivalent `MatchingEngine` paths because they skip engine orchestration, trade handling, and side routing.
- `bestPrice` and `getBestOrder` are the cheapest read paths.
- `findOrder` is faster after moving `orderNodesById_` to `std::unordered_map`.
- Partial `reduceOrderQuantity` and same-price `modifyOrder` are the fastest write-style operations because the order stays in place.
- Exact-fill reduction is slower than partial reduction because it also removes the order node and may clean up the price level.
- Price-change modify is slower than same-price modify because it relinks the order into another level.

### Depth Snapshots
![OrderBookSide getDepth latency](./docs/benchmark_graphs/depth_latency.svg)

![OrderBookSide getDepth throughput](./docs/benchmark_graphs/depth_throughput.svg)

Findings:
- `getDepth(10)` is reasonable for shallow display-style snapshots.
- `getDepth(100)` scales up visibly because it walks more levels and writes more `LevelSnapshot` entries.
- `getDepth(1000)` is the slowest measured side API because it materializes a large snapshot vector.
- Depth benchmarks should be compared separately from order mutation benchmarks because they are read-heavy and allocation-sensitive.

## CI/CD
- CI workflow: [`.github/workflows/ci.yml`](./.github/workflows/ci.yml)
- Coverage workflow: [`.github/workflows/coverage.yml`](./.github/workflows/coverage.yml)
- Runs on every push and pull request.
- Coverage artifacts (`.gcov` + summary) are uploaded in the Coverage workflow.
