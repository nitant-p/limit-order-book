#pragma once

#include <cstddef>
#include <memory>
#include <vector>

struct Order;
struct OrderNode;

class OrderNodePool {
public:
    explicit OrderNodePool(std::size_t capacity);
    ~OrderNodePool();

    OrderNodePool(const OrderNodePool&) = delete;
    OrderNodePool& operator=(const OrderNodePool&) = delete;

    OrderNodePool(OrderNodePool&&) = delete;
    OrderNodePool& operator=(OrderNodePool&&) = delete;

    OrderNode* acquire(const Order& order);
    bool release(OrderNode* node);

    bool owns(const OrderNode* node) const noexcept;
    bool hasAvailable() const noexcept;

    std::size_t capacity() const noexcept;
    std::size_t activeCount() const noexcept;
    std::size_t availableCount() const noexcept;

private:
    void resetNode(OrderNode& node) noexcept;

    std::unique_ptr<OrderNode[]> nodes_;
    std::vector<OrderNode*> freeNodes_;
    std::vector<bool> inUse_;
    std::size_t capacity_ = 0;
    std::size_t activeCount_ = 0;
};
