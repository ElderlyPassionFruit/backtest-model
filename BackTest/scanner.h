#pragma once

#include "completed_transaction.h"
#include "order.h"

#include <string>

class Scanner {
public:
    Scanner() = default;
    void ReadAll(const std::string& path_orderbook, const std::string& path_transactions);
    void ReadOrderBook(const std::string& path_orderbook);
    void ReadTransactions(const std::string& path_transactions);
    const std::vector<TLimitVector>& GetAsk() const;
    const std::vector<TLimitVector>& GetBid() const;
    const std::vector<CompletedTransaction>& GetTransactions() const;

private:
    uint64_t ToInt(const std::string& s, bool use_precision = true) const;
    bool ToBool(const std::string& s) const;
    std::vector<std::string> Split(const std::string& line, const char delimiter = ',') const;
    void TokenizeOrders(const std::string& line);
    void TokenizeTransactions(const std::string& line);
    std::vector<TLimitVector> ask_, bid_;
    std::vector<CompletedTransaction> transactions_;
};