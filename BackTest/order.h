#pragma once

#include "completed_transaction.h"

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <vector>

enum OrderTypes {
    ASK,  // this means, that owner of this order want to cell his asset
    BID   // this means, that owner of this order want to buy new asset
};

std::string ToString(const OrderTypes& order_type);

class BaseOrder {
public:
    virtual ~BaseOrder() = default;
    BaseOrder(const uint64_t& order_id, const uint64_t& submit_timestamp,
              const OrderTypes& order_type, const uint64_t& volume);
    uint64_t GetOrderId() const;
    uint64_t GetSubmitTimestamp() const;
    OrderTypes GetOrderType() const;
    uint64_t GetVolume() const;
    uint64_t GetRemainingVolume() const;
    const TTransactionVector& GetFilling() const;
    void AddTransaction(const TTransaction& completed_transaction);
    bool IsClosed() const;
    void SetVolume(uint64_t new_volume);
    long double GetAveragePrice() const;
    virtual void Print(bool print_name = true) const = 0;

protected:
    uint64_t order_id_;
    uint64_t submit_timestamp_;
    OrderTypes order_type_;
    uint64_t volume_;
    uint64_t remaining_volume_;
    TTransactionVector filling_;
};

class MarketOrder : public BaseOrder {
public:
    MarketOrder(const uint64_t& order_id, const uint64_t& submit_timestamp,
                const OrderTypes& order_type, const uint64_t& volume);
    void Print(bool print_name = true) const override;
};

class LimitOrder : public BaseOrder {
public:
    LimitOrder(const uint64_t& order_id, const uint64_t& submit_timestamp,
               const OrderTypes& order_type, const uint64_t& volume, const uint64_t& price_limit);
    uint64_t GetPriceLimit() const;
    void Print(bool print_name = true) const override;

private:
    uint64_t price_limit_;
};

using TBase = std::shared_ptr<BaseOrder>;
using TLimit = std::shared_ptr<LimitOrder>;
using TMarket = std::shared_ptr<MarketOrder>;
using TBaseVector = std::vector<TBase>;
using TLimitVector = std::vector<TLimit>;
using TMarketVector = std::vector<TMarket>;

struct AskLimitOrderComparator {
    bool operator()(const TLimit& lhs, const TLimit& rhs) const;
};

struct BidLimitOrderComparator {
    bool operator()(const TLimit& lhs, const TLimit& rhs) const;
};

using TAskLimitSet = std::set<TLimit, AskLimitOrderComparator>;
using TBidLimitSet = std::set<TLimit, BidLimitOrderComparator>;