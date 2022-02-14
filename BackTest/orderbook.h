#pragma once

#include "order.h"

#include <memory>
#include <set>
#include <vector>

class OrderBook {
public:
    OrderBook() = default;
    void UpdateOrderBook(const TLimitVector& new_ask, const TLimitVector& new_bid);
    void AddUserLimitOrder(const uint64_t& order_id, const uint64_t& submit_timestamp,
                           const OrderTypes& order_type, const uint64_t& volume,
                           const uint64_t& price_limit);
    void CompleteUserMarketOrder(const uint64_t& order_id, const uint64_t& submit_timestamp,
                                 const OrderTypes& order_type, const uint64_t& volume);
    void CompleteMarketTransaction(const CompletedTransaction& transaction);
    uint64_t AddNewOrder();
    void RemoveOrder(const uint64_t& order_id);
    TBase GetOrderInfo(const uint64_t& order_id) const;
    const TAskLimitSet& GetAsk() const;
    const TBidLimitSet& GetBid() const;
    const TLimitVector& GetUserLimitAsk() const;
    const TLimitVector& GetUserLimitBid() const;
    const TMarketVector& GetUserMarketAsk() const;
    const TMarketVector& GetUserMarketBid() const;
    const TTransactionVector& GetMarketTransactions() const;
    void Print(bool print_name = true) const;

private:
    template <typename TLimitSet>
    void UpdateOrders(TLimitVector cur_orders, TLimitSet& old_orderbook,
                      TLimitVector& historical_orders);
    template <typename TLimitSet>
    void CompleteUserMarketOrder(TMarket market_order, TLimitSet& orders,
                                 const bool is_buyer_maker);
    template <typename TLimitSet>
    void CompleteMarketTransaction(const CompletedTransaction& transaction, TLimitSet& orders);
    TAskLimitSet ask_;
    TBidLimitSet bid_;
    TLimitVector historical_ask_, historical_bid_;
    TLimitVector user_limit_ask_, user_limit_bid_;
    TMarketVector user_market_ask_, user_market_bid_;
    TTransactionVector market_transactions_;
    TBaseVector all_user_orders_;
};
