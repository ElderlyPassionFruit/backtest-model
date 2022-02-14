#pragma once

#include "orderbook.h"
#include "scanner.h"

#include <optional>
#include <queue>
#include <utility>

struct ForRemove {
    uint64_t remove_timestamp;
    uint64_t order_id;
    ForRemove() = default;
    ForRemove(const uint64_t& remove_timestamp, const uint64_t& order_id);
};

struct ForPNL {
    int64_t total_cash;
    int64_t total_asset;
    uint64_t timestamp;
    ForPNL() = default;
    ForPNL(const int64_t& total_cash, const int64_t& total_asset, const uint64_t& timestamp);
    void Print(bool print_name = true) const;
};

class BackTest {
public:
    BackTest() = default;
    // time in ms
    // comission is price * value * comission / 10^4
    BackTest(const std::string& path_orderbook, const std::string& path_transactions,
             uint64_t limit_order_fee = 0, uint64_t market_order_fee = 0,
             uint64_t post_latency = 100, uint64_t cancel_latency = 100,
             uint64_t call_frequency = 100);

    uint64_t ProcessTimeInterval(const uint64_t& step);
    TBase GetOrderInfo(const uint64_t& order_id) const;
    std::optional<uint64_t> SendLimitOrder(const OrderTypes& order_type, const uint64_t& volume,
                                           const uint64_t& price_limit);
    bool WithdrawLimitOrder(uint64_t order_id);
    std::optional<uint64_t> SendMarketOrder(const OrderTypes& order_type, const uint64_t& volume);
    uint64_t GetCurrentTimestamp() const;
    const TAskLimitSet& GetAsk() const;
    const TBidLimitSet& GetBid() const;
    const TLimitVector& GetUserLimitAsk() const;
    const TLimitVector& GetUserLimitBid() const;
    const TMarketVector& GetUserMarketAsk() const;
    const TMarketVector& GetUserMarketBid() const;
    const TTransactionVector& GetCompletedTrades() const;
    std::pair<TAskLimitSet, TBidLimitSet> GetOrderBook() const;
    uint64_t GetOrderPosition(const uint64_t& order_id) const;
    ForPNL GetPNL() const;
    void PrintOrderBook(bool print_name = true) const;

private:
    bool ProcessQueue();

    template <typename TSet, typename TValue>
    uint64_t GetOrder(const TSet& orders, const TValue& order) const;

    uint64_t limit_order_fee_;
    uint64_t market_order_fee_;
    uint64_t post_latency_;
    uint64_t cancel_latency_;
    uint64_t call_frequency_;
    OrderBook orderbook_;
    uint64_t current_timestamp_;
    uint64_t last_call_;
    std::vector<TLimitVector> historical_ask_, historical_bid_;
    std::vector<CompletedTransaction> historical_transactions_;
    std::queue<LimitOrder> queue_limit_orders_;
    std::queue<MarketOrder> queue_market_orders_;
    std::queue<ForRemove> queue_remove_orders_;
    uint64_t orders_position_;
    uint64_t transactions_position_;
    static const uint64_t percent_base_ = 10000;
};