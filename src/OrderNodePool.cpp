//
// Created by Nitant Panicker on 31/5/26.
//
#include "../include/OrderNodePool.h"
#include <cassert>
#include "../include/OrderBookSide.h"

using namespace std;

OrderNodePool::OrderNodePool(std::size_t capacity): capacity_(capacity) {
   nodes_ = std::make_unique<OrderNode[]>(capacity);
   freeNodes_ = vector<OrderNode*>(capacity);
   inUse_ = vector<bool>(capacity);
   for (size_t i = 0; i < capacity; ++i) {
      nodes_[i].poolIndex = i;
      freeNodes_[i] = &nodes_[i];
      inUse_[i] = false;
   }
}

OrderNodePool::~OrderNodePool() = default;

OrderNode* OrderNodePool::acquire(const Order &order) {
   if (freeNodes_.empty()) return nullptr;

   OrderNode* nodePtr = freeNodes_.back();
   assert(!inUse_[nodePtr->poolIndex]);

   nodePtr->order = order;
   nodePtr->next = nullptr;
   nodePtr->prev = nullptr;
   nodePtr->priceLevel = nullptr;

   freeNodes_.pop_back();
   inUse_[nodePtr->poolIndex] = true;
   ++activeCount_;

   return nodePtr;
}

bool OrderNodePool::release(OrderNode* node) {
   if (node == nullptr) return false;
   if (!owns(node)) return false;

   size_t index = node->poolIndex;
   if (index >= capacity_) return false;

   if (!inUse_[index]) return false;

   resetNode(*node);

   inUse_[index] = false;
   --activeCount_;

   freeNodes_.push_back(node);
   return true;
}

bool OrderNodePool::owns(const OrderNode* node) const noexcept {
   if (node == nullptr) return false;
   if (nodes_ == nullptr) return false;

   const OrderNode* start = nodes_.get();
   const OrderNode* end = start + capacity_;

   return node >= start && node < end;
}

void OrderNodePool::resetNode(OrderNode& node) noexcept {
   // clear values
   node.next = nullptr;
   node.prev = nullptr;
   node.priceLevel = nullptr;
}

bool OrderNodePool::hasAvailable() const noexcept {
   return capacity_ - activeCount_ > 0;
}

std::size_t OrderNodePool::capacity() const noexcept {
   return capacity_;
}

std::size_t OrderNodePool::activeCount() const noexcept {
   return activeCount_;
}

std::size_t OrderNodePool::availableCount() const noexcept {
   return capacity_ - activeCount_;
}

