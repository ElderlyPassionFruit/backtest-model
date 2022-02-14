#include "completed_transaction.h"

#include <iostream>

// CompletedTransaction

CompletedTransaction::CompletedTransaction(const uint64_t& transaction_timestamp,
                                           const uint64_t& volume, const uint64_t& price,
                                           const bool& is_buyer_maker)
    : transaction_timestamp_(transaction_timestamp),
      volume_(volume),
      price_(price),
      is_buyer_maker_(is_buyer_maker) {
}

uint64_t CompletedTransaction::GetTransactionTimestamp() const {
    return transaction_timestamp_;
}

uint64_t CompletedTransaction::GetVolume() const {
    return volume_;
}

uint64_t CompletedTransaction::GetPrice() const {
    return price_;
}

bool CompletedTransaction::GetIsBuyerMaker() const {
    return is_buyer_maker_;
}

bool CompletedTransaction::operator<(const CompletedTransaction& other) const {
    return GetTransactionTimestamp() < other.GetTransactionTimestamp();
}

void CompletedTransaction::Print(bool print_name) const {
    if (print_name) {
        std::cerr << "CompletedTransaction:" << std::endl;
    }
    std::cerr << "transaction_timestamp = " << GetTransactionTimestamp()
              << " volume = " << GetVolume() << " price = " << GetPrice()
              << " is_buyer_maker = " << GetIsBuyerMaker() << std::endl;
}
