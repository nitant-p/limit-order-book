# Order Book Project Milestones

## 1. Project Scope
This document defines versioned requirements for a C++ limit order book and matching engine project.
The milestones progress from correctness-first foundations to realistic exchange features and maintainable architecture.

## 2. Common Definitions
### 2.1 Core Terms
- `Order`: instruction to buy or sell a quantity at a price.
- `Resting order`: unmatched quantity stored in the book.
- `Incoming order`: newly submitted order being processed.
- `Trade`: execution event produced by matching incoming and resting orders.
- `Best bid`: highest buy price currently resting.
- `Best ask`: lowest sell price currently resting.
- `Spread`: `bestAsk - bestBid` when both sides are non-empty.

### 2.2 Side and Match Rules
- `BUY` matches `SELL` when `buy.price >= sell.price`.
- `SELL` matches `BUY` when `sell.price <= buy.price`.

### 2.3 Global Constraints
- Emphasize correctness before optimization.
- Add features only in the milestone where they are introduced.
- Preserve deterministic behavior for repeatable tests.

## 3. Coding and Testing Standards (All Versions)
### 3.1 Coding Do
- Use clear domain names: `Order`, `Trade`, `MatchingEngine`, `OrderBookSide`.
- Keep interfaces minimal and explicit.
- Use `const` correctness where possible.
- Separate declarations and definitions (`include/` and `src/`).
- Add unit tests for each required behavior.

### 3.2 Coding Do Not
- Do not add premature micro-optimizations.
- Do not introduce concurrency before required.
- Do not use global mutable state for engine data.
- Do not hide business logic in I/O code.

### 3.3 Testing Do
- Use behavior-driven test names.
- Validate state transitions and emitted trades.
- Cover no-match, full-fill, partial-fill, leftover, and edge ordering cases.

### 3.4 Testing Do Not
- Do not rely only on console output for correctness.
- Do not write tests that depend on execution timing.

---

## Version 1: Naive Single-Match Limit Engine
### Objective
Implement a minimal working matching engine for limit orders with at most one match per incoming order.

### Required Data Model
- `Order` fields:
  - `int id`
  - `Side side` (`BUY` or `SELL`)
  - `int price`
  - `int quantity`

### Required Book Structure
- `std::vector<Order> buyOrders`
- `std::vector<Order> sellOrders`
- No sorting requirement.

### Required Processing Behavior
- Process one incoming order at a time.
- Choose opposite side to scan:
  - incoming `BUY` scans `sellOrders`
  - incoming `SELL` scans `buyOrders`
- Find first compatible resting order by sequential scan.
- Execute at most one trade:
  - `tradeQty = min(incoming.quantity, resting.quantity)`
  - decrement both quantities by `tradeQty`
- Stop scanning after first match attempt.
- If incoming quantity remains, append incoming order to its own side.
- Remove zero-quantity resting orders.

### Required Output
- For matched order: print trade details with buy ID, sell ID, price, quantity.
- For no match: print message that order was added to book.
- Print current buy and sell books after processing each order.

### Do
- Keep implementation straightforward and readable.
- Implement helper methods for match check, quantity update, and cleanup.
- Add tests for all V1 done criteria.

### Do Not
- Do not implement multiple matches for one incoming order.
- Do not implement market orders.
- Do not implement cancel/modify.
- Do not implement price-time priority.
- Do not introduce persistence or networking.

### Definition of Done
- No match stores order correctly.
- Exact match removes both orders.
- Partial fill updates quantities correctly.
- Incoming leftover is stored correctly.
- Resting zero quantity is removed.
- Only one match occurs per incoming order.

---

## Version 2: Best-Price Selection (Single Match)
### Objective
Replace first-compatible selection with best-price selection while keeping one trade per incoming order.

### Required Behavior
- Incoming `BUY` selects lowest-price compatible `SELL`.
- Incoming `SELL` selects highest-price compatible `BUY`.
- Still execute at most one trade per incoming order.
- Maintain V1 quantity and cleanup semantics.

### Data Structure Requirement
- Continue with `std::vector<Order>` on both sides.
- Perform full scan to identify best candidate.

### Do
- Implement explicit best-candidate search helper.
- Preserve deterministic tie handling (document policy).
- Add tests proving best-price choice over first-found behavior.

### Do Not
- Do not add multi-match loop yet.
- Do not add sequence/time priority yet.
- Do not switch to complex containers yet.

### Definition of Done
- Matches always use best compatible opposing price.
- Existing V1 behaviors remain correct.

---

## Version 3: Multi-Match Loop and Trade List
### Objective
Allow one incoming order to generate multiple trades until filled or blocked.

### Required Behavior
- While incoming quantity > 0:
  - find best compatible opposing order
  - execute trade
  - remove depleted resting orders
- Stop when no compatible resting orders remain.
- If incoming quantity remains, store as resting order.

### New Domain Object
- Introduce `Trade` record containing at minimum:
  - buy order ID
  - sell order ID
  - execution price
  - execution quantity

### Do
- Return or capture full trade list per processed order.
- Add tests for book sweep across multiple levels.
- Validate multiple partial fills in one call.

### Do Not
- Do not implement time-priority tie-break yet.
- Do not optimize for latency yet.

### Definition of Done
- Multi-level fills execute in correct order under Version 2 best-price rules.
- Generated trade sequence is correct and complete.

---

## Version 4: Price-Time Priority
### Objective
Apply FIFO priority among orders at the same price level.

### Required Behavior
- Add monotonic sequence/timestamp field to order entry.
- Candidate comparison order:
  - price priority first
  - if equal price, older sequence first
- Preserve Version 3 multi-match behavior.

### Do
- Encapsulate comparator logic.
- Add tie-case tests with identical prices and different sequence values.

### Do Not
- Do not reorder fills in violation of FIFO at same price.
- Do not collapse sequence generation into non-deterministic sources.

### Definition of Done
- Equal-price orders always fill in arrival order.

---

## Version 5: Price-Level Book Representation
### Objective
Refactor book storage to explicit price levels and queues.

### Required Structure
- Buy side grouped by price.
- Sell side grouped by price.
- Within each price, preserve FIFO queue semantics.

### Suggested Containers
- `std::map<int, std::deque<Order>>` or equivalent.
- Buy side configured for efficient best-bid retrieval.
- Sell side configured for efficient best-ask retrieval.

### Required Behavior
- Best-price lookup should use level structure, not full-order scan.
- Matching logic must remain equivalent to Version 4.

### Do
- Introduce `OrderBookSide` abstraction if helpful.
- Add tests validating depth and queue behavior by price level.

### Do Not
- Do not change business rules while refactoring storage.
- Do not sacrifice correctness for data-structure complexity.

### Definition of Done
- Engine behavior matches Version 4 outcomes with new internal structure.
- Depth representation is visible and testable by level.

---

## Version 6: Order Cancel by ID
### Objective
Support cancellation of resting orders by order ID.

### Required Behavior
- Accept cancel request with target order ID.
- Remove matching resting order if present.
- Return clear status: success/failure-not-found.

### Required Consistency
- Book state must remain valid after cancel.
- Any auxiliary indices must stay synchronized.

### Do
- Introduce ID lookup map if needed.
- Add tests for cancel existing, cancel missing, cancel after partial fill.

### Do Not
- Do not allow cancel of already fully filled orders as successful.
- Do not leave dangling references/iterators after removal.

### Definition of Done
- Cancel correctness is deterministic and validated by tests.

---

## Version 7: Market Orders
### Objective
Support market buy and market sell orders.

### Required Behavior
- Market buy consumes best asks until filled or sell side empty.
- Market sell consumes best bids until filled or buy side empty.
- Unfilled remainder of market order is not stored by default.

### Required Modeling
- Add order type field (for example `LIMIT`, `MARKET`).
- Keep matching semantics for existing limit orders unchanged.

### Do
- Add explicit slippage-oriented tests for thin books.
- Validate that market orders ignore limit price constraints.

### Do Not
- Do not allow market orders to rest in the book unless explicitly designed.
- Do not break prior limit-order behavior.

### Definition of Done
- Market orders execute against available liquidity correctly.
- Remaining quantity behavior is explicit and tested.

---

## Version 8: Architecture and Performance Maturity
### Objective
Stabilize production-style architecture, testability, and performance visibility.

### Required Scope
- Separate concerns:
  - matching logic
  - book-side representation
  - input/adapters
  - trade/event output
- Expand automated test coverage (unit + scenario).
- Add benchmark harness for throughput and latency comparisons.

### Required Quality Gates
- Repeatable tests for all prior-version behaviors.
- Basic profiling data for critical paths.
- Clear module boundaries and documented invariants.

### Do
- Refactor for maintainability without changing external behavior.
- Compare alternative internal structures with measured metrics.

### Do Not
- Do not optimize without benchmark evidence.
- Do not merge architectural changes without regression tests.

### Definition of Done
- Engine remains correct across all prior requirements.
- Codebase supports extension and measurable performance analysis.

---

## 4. Milestone Progression Rules
- A milestone is complete only when all required behaviors and done criteria pass tests.
- New milestone work must not regress previous milestone tests.
- Implementation details may evolve; externally required behavior for completed milestones must remain stable.

## 5. Recommended Delivery Pattern per Milestone
- Step 1: finalize explicit acceptance tests.
- Step 2: implement minimal code to satisfy tests.
- Step 3: refactor only after tests pass.
- Step 4: document assumptions and invariants.
