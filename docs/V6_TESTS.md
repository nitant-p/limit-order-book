# Version 6 Test Specification

## Scope
Version 6 introduces cancel-by-order-id behavior for resting orders in the book.

## V6 Requirements Covered
1. Cancel request by `orderId` is accepted and processed.
2. Existing resting order can be removed from the correct side and price queue.
3. Non-existent or already-filled `orderId` does not mutate the book.
4. Cancelling a partially-filled resting order removes its remaining quantity.
5. Removing the last order at a price level removes only that level.
6. Cancellation must not mutate unrelated side/levels.
7. API contract should return explicit success/failure status (`bool`).

## Test Cases
1. `MatchingEngineV6.CancelOrderShouldReturnBoolStatus`
   Verifies the cancel API return type contract is `bool`.
2. `MatchingEngineV6.CancelOnEmptyBook_DoesNotMutate`
   Cancelling with no resting orders leaves both books empty.
3. `MatchingEngineV6.CancelExistingBuyOrder_RemovesOrderIdFromBook`
   Existing buy order id is removed from its level.
4. `MatchingEngineV6.CancelExistingSellOrder_RemovesOrderIdFromBook`
   Existing sell order id is removed from its level.
5. `MatchingEngineV6.CancelMiddleOrderAtSamePrice_RemovesOnlyTargetId`
   Cancelling an in-level non-front order removes only that id and preserves remaining order ids.
6. `MatchingEngineV6.CancelExistingSellOrder_SamePriceQueueRemainingIdPreserved`
   Cancelling front of a same-price queue keeps later ids in FIFO order.
7. `MatchingEngineV6.CancelMissingOrderId_DoesNotMutateBook`
   Unknown id is a no-op.
8. `MatchingEngineV6.CancelTwice_SecondCancelDoesNotMutateBook`
   Repeated cancel on same id after first removal is a no-op.
9. `MatchingEngineV6.CancelAfterPartialFill_RemovesRestingRemainder_SellSide`
   Partial-fill remainder on sell side is removable.
10. `MatchingEngineV6.CancelAfterPartialFill_RemovesRestingRemainder_BuySide`
    Partial-fill remainder on buy side is removable.
11. `MatchingEngineV6.CancelFullyFilledOrderId_NoMutation`
    Cancelling an already-fully-filled id is a no-op.
12. `MatchingEngineV6.CancelLastOrderAtPriceLevel_RemovesOnlyThatLevel`
    Price level cleanup after cancel works correctly.
13. `MatchingEngineV6.CancelDoesNotAffectOppositeSideBook`
    Cancel on one side does not change opposite-side levels.

## How To Run V6 Tests
```bash
cmake -S . -B build
cmake --build build --target matching_engine_v6_tests
ctest --test-dir build -R MatchingEngineV6 --output-on-failure
```

## Coverage Workflow
Coverage tooling is gcov-based.

Configure with coverage flags enabled:
```bash
cmake -S . -B build-coverage -DENABLE_COVERAGE=ON
cmake --build build-coverage --target coverage
```

If tests are still failing and you still want intermediate coverage artifacts, run:
```bash
./scripts/generate_coverage.sh build
```

Artifacts produced under:
- `build-coverage/coverage/*.gcov`
- `build-coverage/coverage/coverage_summary.txt`
