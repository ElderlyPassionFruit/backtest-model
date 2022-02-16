#include "backtest.h"

#include <algorithm>
#include <iostream>

// ForRemove

ForRemove::ForRemove(const uint64_t& remove_timestamp, const uint64_t& order_id)
    : remove_timestamp(remove_timestamp), order_id(order_id) {
}

// ForPNL

ForPNL::ForPNL(const int64_t& total_cash, const int64_t& total_asset, const uint64_t& timestamp)
    : total_cash(total_cash), total_asset(total_asset), timestamp(timestamp) {
}

void ForPNL::Print(bool print_name) const {
    if (print_name) {
        std::cerr << "PNL:" << std::endl;
    }
    std::cerr << "timestamp = " << timestamp << " total_cash = " << total_cash
              << " total_asset = " << total_asset << std::endl;
}

// BackTest

BackTest::BackTest(const std::string& path_orderbook, const std::string& path_transactions,
                   uint64_t limit_order_fee, uint64_t market_order_fee, uint64_t post_latency,
                   uint64_t cancel_latency, uint64_t call_frequency)
    : limit_order_fee_(limit_order_fee),
      market_order_fee_(market_order_fee),
      post_latency_(post_latency),
      cancel_latency_(cancel_latency),
      call_frequency_(call_frequency),
      orderbook_(),
      current_timestamp_(0),
      last_call_(0),
      historical_ask_(),
      historical_bid_(),
      historical_transactions_(),
      queue_limit_orders_(),
      queue_market_orders_(),
      queue_remove_orders_(),
      orders_position_(0),
      transactions_position_(0) {
    Scanner scanner;
    scanner.ReadAll(path_orderbook, path_transactions);
    std::cerr << "Data read successfully." << std::endl;
    historical_ask_ = scanner.GetAsk();
    historical_bid_ = scanner.GetBid();
    historical_transactions_ = scanner.GetTransactions();
}

bool BackTest::ProcessQueue() {
    uint64_t orders_time = orders_position_ < historical_ask_.size()
                               ? historical_ask_[orders_position_][0]->GetSubmitTimestamp()
                               : -1;
    uint64_t transactions_time =
        transactions_position_ < historical_transactions_.size()
            ? historical_transactions_[transactions_position_].GetTransactionTimestamp()
            : -1;
    uint64_t limit =
        !queue_limit_orders_.empty() ? queue_limit_orders_.front().GetSubmitTimestamp() : -1;
    uint64_t market =
        !queue_market_orders_.empty() ? queue_market_orders_.front().GetSubmitTimestamp() : -1;
    uint64_t remove =
        !queue_remove_orders_.empty() ? queue_remove_orders_.front().remove_timestamp : -1;

    uint64_t min_value = std::min({orders_time, transactions_time, limit, market, remove});
    if (min_value > current_timestamp_) {
        return false;
    }
    if (min_value == orders_time) {
        orderbook_.UpdateOrderBook(historical_ask_[orders_position_],
                                   historical_bid_[orders_position_]);
        ++orders_position_;
    } else if (min_value == transactions_time) {
        orderbook_.CompleteMarketTransaction(historical_transactions_[transactions_position_++]);
    } else if (min_value == limit) {
        auto order = queue_limit_orders_.front();
        queue_limit_orders_.pop();
        orderbook_.AddUserLimitOrder(order.GetOrderId(), order.GetSubmitTimestamp(),
                                     order.GetOrderType(), order.GetVolume(),
                                     order.GetPriceLimit());
    } else if (min_value == market) {
        auto order = queue_market_orders_.front();
        queue_market_orders_.pop();
        orderbook_.CompleteUserMarketOrder(order.GetOrderId(), order.GetSubmitTimestamp(),
                                           order.GetOrderType(), order.GetVolume());
    } else if (min_value == remove) {
        auto order_id = queue_remove_orders_.front().order_id;
        queue_remove_orders_.pop();
        orderbook_.RemoveOrder(order_id);
    }
    return true;
}

uint64_t BackTest::ProcessTimeInterval(const uint64_t& step) {
    current_timestamp_ += step;
    while (ProcessQueue()) {
    }
    return current_timestamp_;
}

uint64_t BackTest::ProcessBeforeUnlock() {
    if (last_call_ + call_frequency_ <= current_timestamp_) {
        return current_timestamp_;
    } else {
        return BackTest::ProcessTimeInterval(last_call_ + call_frequency_ - current_timestamp_);
    }
}

TBase BackTest::GetOrderInfo(const uint64_t& order_id) const {
    return orderbook_.GetOrderInfo(order_id);
}

std::optional<uint64_t> BackTest::SendLimitOrder(const OrderTypes& order_type,
                                                 const uint64_t& volume,
                                                 const uint64_t& price_limit) {
    if (last_call_ + call_frequency_ > current_timestamp_) {
        return std::nullopt;
    }
    last_call_ = current_timestamp_;
    uint64_t order_id = orderbook_.AddNewOrder();
    queue_limit_orders_.push(
        LimitOrder(order_id, current_timestamp_ + post_latency_, order_type, volume, price_limit));
    return order_id;
}

bool BackTest::WithdrawLimitOrder(uint64_t order_id) {
    if (last_call_ + call_frequency_ > current_timestamp_) {
        return false;
    }
    last_call_ = current_timestamp_;
    queue_remove_orders_.push(ForRemove(current_timestamp_ + cancel_latency_, order_id));
    return true;
}

std::optional<uint64_t> BackTest::SendMarketOrder(const OrderTypes& order_type,
                                                  const uint64_t& volume) {
    if (last_call_ + call_frequency_ > current_timestamp_) {
        return std::nullopt;
    }
    last_call_ = current_timestamp_;
    uint64_t order_id = orderbook_.AddNewOrder();
    queue_market_orders_.push(
        MarketOrder(order_id, current_timestamp_ + post_latency_, order_type, volume));
    return order_id;
}

const TAskLimitSet& BackTest::GetAsk() const {
    return orderbook_.GetAsk();
}

const TBidLimitSet& BackTest::GetBid() const {
    return orderbook_.GetBid();
}

uint64_t BackTest::GetCurrentTimestamp() const {
    return current_timestamp_;
}

const TLimitVector& BackTest::GetUserLimitAsk() const {
    return orderbook_.GetUserLimitAsk();
}

const TLimitVector& BackTest::GetUserLimitBid() const {
    return orderbook_.GetUserLimitBid();
}

const TMarketVector& BackTest::GetUserMarketAsk() const {
    return orderbook_.GetUserMarketAsk();
}

const TMarketVector& BackTest::GetUserMarketBid() const {
    return orderbook_.GetUserMarketBid();
}

const TTransactionVector& BackTest::GetCompletedTrades() const {
    return orderbook_.GetMarketTransactions();
}

std::pair<TAskLimitSet, TBidLimitSet> BackTest::GetOrderBook() const {
    return {orderbook_.GetAsk(), orderbook_.GetBid()};
}

template <typename TSet, typename TValue>
uint64_t BackTest::GetOrder(const TSet& orders, const TValue& order) const {
    uint64_t answer = 0;
    for (const auto& cur : orders) {
        if (cur->GetOrderId() == order->GetOrderId()) {
            return answer;
        }
        ++answer;
    }
    return -1;
}

uint64_t BackTest::GetOrderPosition(const uint64_t& order_id) const {
    if (!GetOrderInfo(order_id)) {
        return -1;
    }
    auto order = static_cast<LimitOrder*>(GetOrderInfo(order_id).get());
    if (order->GetOrderType() == ASK) {
        return GetOrder(GetAsk(), order);
    } else if (order->GetOrderType() == BID) {
        return GetOrder(GetBid(), order);
    } else {
        throw std::runtime_error("BackTest::GetOrderPosition - Incorrect order_type.");
    }
}

ForPNL BackTest::GetPNL() const {
    int64_t total_cash = 0, total_asset = 0;
    for (const auto& order : GetUserLimitAsk()) {
        for (const auto& transaction : order->GetFilling()) {
            total_cash += transaction->GetPrice() * transaction->GetVolume() *
                          (percent_base_ - limit_order_fee_) / percent_base_;
            total_asset -= transaction->GetVolume();
        }
    }
    for (const auto& order : GetUserMarketAsk()) {
        for (const auto& transaction : order->GetFilling()) {
            total_cash += transaction->GetPrice() * transaction->GetVolume() *
                          (percent_base_ - limit_order_fee_) / percent_base_;
            total_asset -= transaction->GetVolume();
        }
    }
    for (const auto& order : GetUserLimitBid()) {
        for (const auto& transaction : order->GetFilling()) {
            total_cash -= transaction->GetPrice() * transaction->GetVolume() *
                          (percent_base_ - market_order_fee_) / percent_base_;
            total_asset += transaction->GetVolume();
        }
    }
    for (const auto& order : GetUserMarketBid()) {
        for (const auto& transaction : order->GetFilling()) {
            total_cash -= transaction->GetPrice() * transaction->GetVolume() *
                          (percent_base_ - market_order_fee_) / percent_base_;
            total_asset += transaction->GetVolume();
        }
    }
    return ForPNL(total_cash, total_asset, GetCurrentTimestamp());
}

void BackTest::PrintOrderBook(bool print_name) const {
    orderbook_.Print(print_name);
}

uint64_t BackTest::GetBestBid() const {
    if (GetBid().empty()) {
        throw std::runtime_error("BackTest::GetBestBid - orderbook_.bid_ have to be non empty.");
    }
    return (*GetBid().begin())->GetPriceLimit();
}

uint64_t BackTest::GetBestAsk() const {
    if (GetAsk().empty()) {
        throw std::runtime_error("BackTest::GetBestAsk - orderbook_.ask_ have to be non empty.");
    }
    return (*GetAsk().begin())->GetPriceLimit();
}

uint64_t BackTest::GetLimitOrderFee() const {
    return limit_order_fee_;
}

uint64_t BackTest::GetMarketOrderFee() const {
    return market_order_fee_;
}

uint64_t BackTest::GetPostLatency() const {
    return post_latency_;
}

uint64_t BackTest::GetCancelLatency() const {
    return cancel_latency_;
}

uint64_t BackTest::GetCallFrequency() const {
    return call_frequency_;
}

uint64_t BackTest::GetLastCall() const {
    return last_call_;
}
