# Order Book (C++)

A C++ learning project that builds a limit-order matching engine incrementally, from a minimal single-match implementation to a more realistic exchange-style order book.

## Project Goal
- Implement a correct, testable order book and matching engine.
- Progress through structured milestones that introduce market microstructure logic and C++ systems design concepts.

## Scope
- Limit order matching engine core.
- Versioned implementation path (V1 → V8).
- Unit-test driven validation of behavior at each milestone.

## Milestones
Version requirements and acceptance criteria are defined in:
- [MILESTONES.md](./MILESTONES.md)

## Build and Test
```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
