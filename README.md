# Order Book (C++)
[![CI](https://github.com/nitant-p/order-book/actions/workflows/ci.yml/badge.svg)](https://github.com/nitant-p/order-book/actions/workflows/ci.yml)
[![Coverage](https://github.com/nitant-p/order-book/actions/workflows/coverage.yml/badge.svg)](https://github.com/nitant-p/order-book/actions/workflows/coverage.yml)

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

## Milestones
Version requirements and acceptance criteria are defined in:
- [MILESTONES.md](./MILESTONES.md)

## Architecture
- Matching engine: [`MatchingEngine.h`](./include/MatchingEngine.h), [`MatchingEngine.cpp`](./src/MatchingEngine.cpp)
- Order model: [`Order.h`](./include/Order.h)
- Trade model: [`Trade.h`](./include/Trade.h)
- Test suite: `tests/` (category-based fixtures)

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

## CI/CD
- CI workflow: [`.github/workflows/ci.yml`](./.github/workflows/ci.yml)
- Coverage workflow: [`.github/workflows/coverage.yml`](./.github/workflows/coverage.yml)
- Runs on every push and pull request.
- Coverage artifacts (`.gcov` + summary) are uploaded in the Coverage workflow.
