#include "order.h"

#include <iostream>
#include <stdexcept>
#include <tuple>

// OrderTypes

std::string ToString(const OrderTypes& order_type) {
    if (order_type == ASK) {
        return "ASK";
    } else if (order_type == BID) {
        return "BID";
    } else {
        throw std::runtime_error("ToString - Incorrect order_type.");
    }
}

// BaseOrder

BaseOrder::BaseOrder(const uint64_t& order_id, const uint64_t& submit_timestamp,
                     const OrderTypes& order_type, const uint64_t& volume)
    : order_id_(order_id),
      submit_timestamp_(submit_timestamp),
      order_type_(order_type),
      volume_(volume),
      remaining_volume_(volume),
      filling_() {
}

uint64_t BaseOrder::GetOrderId() const {
    return order_id_;
}

uint64_t BaseOrder::GetSubmitTimestamp() const {
    return submit_timestamp_;
}

OrderTypes BaseOrder::GetOrderType() const {
    return order_type_;
}

uint64_t BaseOrder::GetVolume() const {
    return volume_;
}

uint64_t BaseOrder::GetRemainingVolume() const {
    return remaining_volume_;
}

const TTransactionVector& BaseOrder::GetFilling() const {
    return filling_;
}

void BaseOrder::AddTransaction(const TTransaction& completed_transaction) {
    if (GetRemainingVolume() < completed_transaction->GetVolume()) {
        throw std::runtime_error(
            "BaseOrder::AddTransaction - Total volume of all transactions can't be greater, than "
            "the initial volume of the order.");
    }
    if (GetSubmitTimestamp() > completed_transaction->GetTransactionTimestamp()) {
        throw std::runtime_error(
            "BaseOrder::AddTransaction - The transaction can't be completed until the order is "
            "created.");
    }
    filling_.emplace_back(completed_transaction);
    remaining_volume_ -= completed_transaction->GetVolume();
}

bool BaseOrder::IsClosed() const {
    return GetRemainingVolume() == 0;
}

void BaseOrder::SetVolume(uint64_t new_volume) {
    volume_ = new_volume;
    remaining_volume_ = new_volume;
    filling_.clear();
}

long double BaseOrder::GetAveragePrice() const {
    if (GetFilling().empty()) {
        return 0;
    } else {
        uint64_t sum = 0, volume = 0;
        for (const auto& transaction : GetFilling()) {
            sum += transaction->GetVolume() * transaction->GetPrice();
            volume += transaction->GetVolume();
        }
        return static_cast<long double>(sum) / volume;
    }
}

// MarketOrder

MarketOrder::MarketOrder(const uint64_t& order_id, const uint64_t& submit_timestamp,
                         const OrderTypes& order_type, const uint64_t& volume)
    : BaseOrder(order_id, submit_timestamp, order_type, volume) {
}

void MarketOrder::Print(bool print_name) const {
    if (print_name) {
        std::cerr << "MarketOrder: " << std::endl;
    }
    std::cerr << "order_id = " << (GetOrderId() == -1 ? "-1" : std::to_string(GetOrderId()))
              << " submit_timestamp = " << GetSubmitTimestamp()
              << " order_type = " << ToString(GetOrderType()) << " volume = " << GetVolume()
              << " remaining_volume = " << GetRemainingVolume() << std::endl;
    std::cerr << "filling: count = " << GetFilling().size() << std::endl;
    for (const auto& completed_transaction : GetFilling()) {
        completed_transaction->Print(false);
    }
}

// LimitOrder

LimitOrder::LimitOrder(const uint64_t& order_id, const uint64_t& submit_timestamp,
                       const OrderTypes& order_type, const uint64_t& volume,
                       const uint64_t& price_limit)
    : BaseOrder(order_id, submit_timestamp, order_type, volume),
      price_limit_(price_limit),
      is_canceled_(false) {
}

uint64_t LimitOrder::GetPriceLimit() const {
    return price_limit_;
}

void LimitOrder::CancelOrder() {
    is_canceled_ = true;
}

bool LimitOrder::IsCanceled() const {
    return is_canceled_;
}

void LimitOrder::Print(bool print_name) const {
    if (print_name) {
        std::cerr << "LimitOrder: " << std::endl;
    }
    std::cerr << "order_id = " << (GetOrderId() == -1 ? "-1" : std::to_string(GetOrderId()))
              << " submit_timestamp = " << GetSubmitTimestamp()
              << " order_type = " << ToString(GetOrderType()) << " volume = " << GetVolume()
              << " remaining_volume = " << GetRemainingVolume()
              << " price_limit = " << GetPriceLimit() << " is_canceled = " << IsCanceled()
              << std::endl;
    std::cerr << "filling: count = " << GetFilling().size() << std::endl;
    for (const auto& completed_transaction : GetFilling()) {
        completed_transaction->Print(false);
    }
}

// AskLimitOrderComparator

bool AskLimitOrderComparator::operator()(const TLimit& lhs, const TLimit& rhs) const {
    if (lhs->GetOrderType() != rhs->GetOrderType()) {
        throw std::runtime_error(
            "AskLimitOrderComparator::operator() - Orders of different types are incomparable.");
    }
    if (lhs->GetPriceLimit() != rhs->GetPriceLimit()) {
        return lhs->GetPriceLimit() < rhs->GetPriceLimit();
    } else if (lhs->GetSubmitTimestamp() != rhs->GetSubmitTimestamp()) {
        return lhs->GetSubmitTimestamp() < rhs->GetSubmitTimestamp();
    } else {
        return lhs->GetOrderId() < rhs->GetOrderId();
    }
}

// BidLimitOrderComparator

bool BidLimitOrderComparator::operator()(const TLimit& lhs, const TLimit& rhs) const {
    if (lhs->GetOrderType() != rhs->GetOrderType()) {
        throw std::runtime_error(
            "BidLimitOrderComparator::operator() - Orders of different types are incomparable.");
    }
    if (lhs->GetPriceLimit() != rhs->GetPriceLimit()) {
        return lhs->GetPriceLimit() > rhs->GetPriceLimit();
    } else if (lhs->GetSubmitTimestamp() != rhs->GetSubmitTimestamp()) {
        return lhs->GetSubmitTimestamp() < rhs->GetSubmitTimestamp();
    } else {
        return lhs->GetOrderId() < rhs->GetOrderId();
    }
}
