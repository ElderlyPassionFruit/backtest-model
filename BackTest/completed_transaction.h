#pragma once

#include <cstdint>
#include <memory>
#include <vector>

class CompletedTransaction {
public:
    CompletedTransaction(const uint64_t& transaction_timestamp, const uint64_t& volume,
                         const uint64_t& price, const bool& is_buyer_maker);
    uint64_t GetTransactionTimestamp() const;
    uint64_t GetVolume() const;
    uint64_t GetPrice() const;
    bool GetIsBuyerMaker() const;
    bool operator<(const CompletedTransaction& other) const;
    void Print(bool print_name = true) const;

private:
    uint64_t transaction_timestamp_;
    uint64_t volume_;
    uint64_t price_;
    bool is_buyer_maker_;
};

using TTransaction = std::shared_ptr<CompletedTransaction>;
using TTransactionVector = std::vector<TTransaction>;
