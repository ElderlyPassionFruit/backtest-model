#include "orderbook.h"

#include <algorithm>
#include <iostream>

// OrderBook

void OrderBook::UpdateOrderBook(const TLimitVector& new_ask, const TLimitVector& new_bid) {
    UpdateOrders(new_ask, ask_, historical_ask_);
    UpdateOrders(new_bid, bid_, historical_bid_);
}

auto find(const TLimitVector& orders, uint64_t price_limit) {
    auto IsEqual = [price_limit](const TLimit& order) {
        return order->GetPriceLimit() == price_limit;
    };
    return find_if(orders.begin(), orders.end(), IsEqual);
}

template <typename TLimitSet>
void OrderBook::UpdateOrders(TLimitVector cur_orders, TLimitSet& old_orderbook,
                             TLimitVector& historical_orders) {
    TLimitSet new_orderbook;
    TLimitVector new_orders;
    for (const auto& order : old_orderbook) {
        if (order->IsClosed()) {
            continue;
        }
        if (order->GetOrderId() != -1) {
            new_orderbook.insert(order);
        } else {
            auto it = find(cur_orders, order->GetPriceLimit());
            if (it == cur_orders.end() || (*it)->GetVolume() > 0) {
                continue;
            }
            TLimit& cur_order = cur_orders[it - cur_orders.begin()];
            uint64_t order_volume = std::min(cur_order->GetVolume(), order->GetVolume());
            new_orders.emplace_back(
                std::make_shared<LimitOrder>(-1, order->GetSubmitTimestamp(), order->GetOrderType(),
                                             order_volume, order->GetPriceLimit()));
            new_orderbook.insert(new_orders.back());
            cur_order->SetVolume(cur_order->GetVolume() - order_volume);
        }
    }
    for (const auto& order : cur_orders) {
        if (order->IsClosed()) {
            continue;
        }
        new_orders.emplace_back(order);
        new_orderbook.insert(new_orders.back());
    }
    old_orderbook = new_orderbook;
    historical_orders = new_orders;
}

void OrderBook::AddUserLimitOrder(const uint64_t& order_id, const uint64_t& submit_timestamp,
                                  const OrderTypes& order_type, const uint64_t& volume,
                                  const uint64_t& price_limit) {
    TLimit limit_order =
        std::make_shared<LimitOrder>(order_id, submit_timestamp, order_type, volume, price_limit);
    all_user_orders_[order_id] = limit_order;
    // Q: Add checker for incorrect price_limit?
    if (order_type == ASK) {
        user_limit_ask_.emplace_back(limit_order);
        ask_.insert(limit_order);
    } else if (order_type == BID) {
        user_limit_bid_.emplace_back(limit_order);
        bid_.insert(limit_order);
    } else {
        throw std::runtime_error("OrderBook::AddUserLimitOrder - Incorrect order_type.");
    }
}

void OrderBook::CompleteUserMarketOrder(const uint64_t& order_id, const uint64_t& submit_timestamp,
                                        const OrderTypes& order_type, const uint64_t& volume) {
    TMarket market_order =
        std::make_shared<MarketOrder>(order_id, submit_timestamp, order_type, volume);
    all_user_orders_[order_id] = market_order;
    if (order_type == ASK) {
        CompleteUserMarketOrder(market_order, bid_, true);
        user_market_ask_.emplace_back(market_order);
    } else if (order_type == BID) {
        CompleteUserMarketOrder(market_order, ask_, false);
        user_market_bid_.emplace_back(market_order);
    } else {
        throw std::runtime_error("OrderBook::CompleteUserMarketOrder - Incorrect order_type.");
    }
    if (!market_order->IsClosed()) {
        throw std::runtime_error("OrderBook::CompleteUserMarketOrder - Order is too big.");
    }
}

template <typename TLimitSet>
void OrderBook::CompleteUserMarketOrder(TMarket market_order, TLimitSet& orders,
                                        const bool is_buyer_maker) {
    for (auto it = orders.begin(); it != orders.end() && !market_order->IsClosed(); ++it) {
        auto cur_pointer = *it;
        if (cur_pointer->GetOrderId() == -1 && !cur_pointer->IsClosed()) {
            uint64_t transaction_volume =
                std::min(market_order->GetRemainingVolume(), cur_pointer->GetRemainingVolume());
            TTransaction transaction = std::make_shared<CompletedTransaction>(
                market_order->GetSubmitTimestamp(), transaction_volume,
                cur_pointer->GetPriceLimit(), is_buyer_maker);
            cur_pointer->AddTransaction(transaction);
            market_order->AddTransaction(transaction);
            market_transactions_.emplace_back(transaction);
        }
    }
}

void OrderBook::CompleteMarketTransaction(const CompletedTransaction& transaction) {
    if (transaction.GetIsBuyerMaker()) {
        CompleteMarketTransaction(transaction, bid_);
    } else {
        CompleteMarketTransaction(transaction, ask_);
    }
}

uint64_t OrderBook::AddNewOrder() {
    all_user_orders_.push_back(nullptr);
    return all_user_orders_.size() - 1;
}

void OrderBook::RemoveOrder(const uint64_t& order_id) {
    auto order = static_cast<LimitOrder*>(all_user_orders_[order_id].get());
    order->CancelOrder();
    auto ptr = std::make_shared<LimitOrder>(order->GetOrderId(), order->GetSubmitTimestamp(),
                                            order->GetOrderType(), order->GetVolume(),
                                            order->GetPriceLimit());
    if (ptr->GetOrderType() == ASK) {
        ask_.erase(ptr);
    } else if (ptr->GetOrderType() == BID) {
        bid_.erase(ptr);
    } else {
        throw std::runtime_error("OrderBook::RemoveOrder - Incorrect order_type.");
    }
}

template <typename TLimitSet>
void OrderBook::CompleteMarketTransaction(const CompletedTransaction& transaction,
                                          TLimitSet& orders) {
    uint64_t current_volume = transaction.GetVolume();

    for (auto it = orders.begin(); it != orders.end() && current_volume > 0; ++it) {
        auto cur_pointer = *it;
        // Q: Maybe there have to be another condition?
        if (!cur_pointer->IsClosed()) {
            uint64_t transaction_volume =
                std::min(current_volume, cur_pointer->GetRemainingVolume());
            TTransaction current_transaction = std::make_shared<CompletedTransaction>(
                transaction.GetTransactionTimestamp(), transaction_volume,
                cur_pointer->GetPriceLimit(), transaction.GetIsBuyerMaker());
            cur_pointer->AddTransaction(current_transaction);
            current_volume -= transaction_volume;
            market_transactions_.emplace_back(current_transaction);
        }
    }
    if (current_volume > 0) {
        throw std::runtime_error("OrderBook::CompleteMarketTransaction - Transaction is too big.");
    }
}

TBase OrderBook::GetOrderInfo(const uint64_t& order_id) const {
    if (order_id >= all_user_orders_.size()) {
        throw std::runtime_error(
            "OrderBook::GetOrderInfo - It is forbidden to request a non-existent transaction.");
    }
    return all_user_orders_[order_id];
}

const TAskLimitSet& OrderBook::GetAsk() const {
    return ask_;
}

const TBidLimitSet& OrderBook::GetBid() const {
    return bid_;
}

const TLimitVector& OrderBook::GetUserLimitAsk() const {
    return user_limit_ask_;
}

const TLimitVector& OrderBook::GetUserLimitBid() const {
    return user_limit_bid_;
}

const TMarketVector& OrderBook::GetUserMarketAsk() const {
    return user_market_ask_;
}

const TMarketVector& OrderBook::GetUserMarketBid() const {
    return user_market_bid_;
}

const TTransactionVector& OrderBook::GetMarketTransactions() const {
    return market_transactions_;
}

void OrderBook::Print(bool print_name) const {
    if (print_name) {
        std::cerr << "OrderBook:" << std::endl;
    }
    std::cerr << "ask = " << std::endl;
    for (const auto& order : GetAsk()) {
        order->Print(false);
    }
    std::cerr << "bid = " << std::endl;
    for (const auto& order : GetBid()) {
        order->Print(false);
    }
    std::cerr << "market_transactions = " << std::endl;
    for (const auto& transaction : GetMarketTransactions()) {
        transaction->Print(false);
    }
}