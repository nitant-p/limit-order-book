# Architecture Diagrams

This directory contains rendered architecture diagrams used by the root README, plus the Mermaid source used to regenerate them.

## Matching Engine
```mermaid
flowchart TB
    Engine["MatchingEngine"]

    subgraph EngineState["Engine state and APIs"]
        Process["processOrder()"]
        Cancel["cancelOrder()"]
        Modify["modifyOrder()"]
        Trades["tradeHistory"]
        SideIndex["orderIdSide: unordered_map&lt;id, Side&gt;"]
    end

    BuyBook["buyBook: OrderBookSide(BUY)"]
    SellBook["sellBook: OrderBookSide(SELL)"]

    subgraph BuyOwned["Buy book owned state"]
        BuyLevels["priceToLevels_: map&lt;int, PriceLevel&gt;"]
        BuyOrders["orderNodesById_: unordered_map&lt;uint64_t, unique_ptr&lt;OrderNode&gt;&gt;"]
    end

    subgraph SellOwned["Sell book owned state"]
        SellLevels["priceToLevels_: map&lt;int, PriceLevel&gt;"]
        SellOrders["orderNodesById_: unordered_map&lt;uint64_t, unique_ptr&lt;OrderNode&gt;&gt;"]
    end

    Engine --> EngineState
    Engine --> BuyBook
    Engine --> SellBook
    BuyBook --> BuyOwned
    SellBook --> SellOwned
```

## Price Levels
```mermaid
flowchart TB
    Map["priceToLevels_: map&lt;int, PriceLevel&gt;"]
    Root["price 100"]
    Left["price 99"]
    Right["price 101"]
    FarRight["price 103"]
    Level["PriceLevel<br/>orderCount<br/>totalQuantity"]
    Head["head"]
    Tail["tail"]
    First["OrderNode"]
    Last["OrderNode"]

    Map --> Root
    Root --"left"--> Left
    Root --"right"--> Right
    Right --"right"--> FarRight

    Root -. "value" .-> Level
    Level --> Head
    Level --> Tail
    Head --> First
    Tail --> Last
```

## Order Nodes
```mermaid
flowchart LR
    IdMap["orderNodesById_: unordered_map&lt;uint64_t, unique_ptr&lt;OrderNode&gt;&gt;"]
    Id1["id 41"]
    Id2["id 42"]
    Id3["id 43"]

    subgraph Orders["Doubly linked order nodes at one price"]
        N1["OrderNode<br/>order id 41"]
        N2["OrderNode<br/>order id 42"]
        N3["OrderNode<br/>order id 43"]

        N1 -- "next" --> N2
        N2 -- "prev" --> N1
        N2 -- "next" --> N3
        N3 -- "prev" --> N2
    end

    Level["PriceLevel"]

    IdMap --> Id1
    IdMap --> Id2
    IdMap --> Id3
    Id1 -. "owns unique_ptr" .-> N1
    Id2 -. "owns unique_ptr" .-> N2
    Id3 -. "owns unique_ptr" .-> N3
    N2 -. "priceLevel" .-> Level
    Level --> Head["head"]
    Level --> Tail["tail"]
    Head --> N1
    Tail --> N3
```
