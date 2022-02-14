#include "scanner.h"

#include <iostream>
#include <fstream>

// Scanner

void Scanner::ReadAll(const std::string& path_orderbook, const std::string& path_transactions) {
    ReadOrderBook(path_orderbook);
    ReadTransactions(path_transactions);
}

void Scanner::ReadOrderBook(const std::string& path_orderbook) {
    std::ifstream in(path_orderbook);
    if (!in.is_open()) {
        throw std::runtime_error(
            "Scanner::ReadOrderbook - Failed to open the file with an orderbook.");
    }
    std::string cur_line;
    getline(in, cur_line);
    while (getline(in, cur_line)) {
        TokenizeOrders(cur_line);
    }
}

void Scanner::ReadTransactions(const std::string& path_transactions) {
    std::ifstream in(path_transactions);
    if (!in.is_open()) {
        throw std::runtime_error(
            "Scanner::ReadTransactions - Failed to open the file with an orderbook.");
    }
    std::string cur_line;
    getline(in, cur_line);
    while (getline(in, cur_line)) {
        TokenizeTransactions(cur_line);
    }
}

uint64_t Scanner::ToInt(const std::string& s, bool use_precition) const {
    static const uint64_t precision = 5;
    uint64_t value = 0;
    bool has_dot = false;
    uint64_t cnt_after_dot = 0;
    for (const auto& c : s) {
        if (c == '.') {
            if (has_dot) {
                throw std::runtime_error(
                    "Scanner::ToInt - There have to be no more than one dot in one string.");
            }
            has_dot = true;
        } else {
            if (c < '0' || c > '9') {
                throw std::runtime_error(
                    "Scanner::ToInt - All characters have to be numbers or dots.");
            }
            if (has_dot) {
                ++cnt_after_dot;
            }
            value = value * 10 + static_cast<uint64_t>(c - '0');
        }
    }
    if (use_precition) {
        while (cnt_after_dot < precision) {
            ++cnt_after_dot;
            value *= 10;
        }
    }
    return value;
}

bool Scanner::ToBool(const std::string& s) const {
    if (s == "True") {
        return true;
    } else if (s == "False") {
        return false;
    } else {
        throw std::runtime_error(
            "Scanner::ToBool - Incorrect s, it have to be equal to True/False.");
    }
}

std::vector<std::string> Scanner::Split(const std::string& line, const char delimiter) const {
    std::vector<std::string> answer;
    std::string cur;
    for (size_t l = 0; l < line.size(); ++l) {
        if (line[l] == delimiter) {
            if (!cur.empty()) {
                answer.emplace_back(cur);
            }
            cur = "";
        } else {
            cur += line[l];
        }
    }
    if (!cur.empty()) {
        answer.emplace_back(cur);
    }
    return answer;
}

void Scanner::TokenizeOrders(const std::string& line) {
    auto blocks = Split(line);

    if (blocks.empty()) {
        return;
    }

    if (blocks.size() != 202) {
        throw std::runtime_error(
            "Scanner::TokenizeOrders - Incorrect number of blocks in the line.");
    }

    static const uint64_t top = 50;
    static const uint64_t timestamp_position = 1;
    static const uint64_t from_ask_price = timestamp_position + 1;
    static const uint64_t from_ask_volume = from_ask_price + top;
    static const uint64_t from_bid_price = from_ask_volume + top;
    static const uint64_t from_bid_volume = from_bid_volume + top;

    TLimitVector to_ask, to_bid;

    uint64_t timestamp = ToInt(blocks[timestamp_position], false);

    for (size_t i = 0; i < top; ++i) {
        uint64_t ask_price = ToInt(blocks[from_ask_price + i]);
        uint64_t ask_volume = ToInt(blocks[from_ask_volume + i]);
        uint64_t bid_price = ToInt(blocks[from_bid_price + i]);
        uint64_t bid_volume = ToInt(blocks[from_bid_volume + i]);
        to_ask.emplace_back(
            std::make_shared<LimitOrder>(-1, timestamp, ASK, ask_volume, ask_price));
        to_bid.emplace_back(
            std::make_shared<LimitOrder>(-1, timestamp, BID, bid_volume, bid_price));
    }

    ask_.emplace_back(to_ask);
    bid_.emplace_back(to_bid);
}

void Scanner::TokenizeTransactions(const std::string& line) {
    auto blocks = Split(line);
    if (blocks.empty()) {
        return;
    }

    if (blocks.size() != 5) {
        throw std::runtime_error(
            "Scanner::TokenizeTransactions - Incorrect number of blocks in the line.");
    }

    static const uint64_t timestamp_position = 1;
    static const uint64_t volume_position = timestamp_position + 1;
    static const uint64_t price_position = volume_position + 1;
    static const uint64_t is_buyer_maker_position = price_position + 1;

    transactions_.emplace_back(ToInt(blocks[timestamp_position], false),
                               ToInt(blocks[price_position]), ToInt(blocks[volume_position]),
                               ToBool(blocks[is_buyer_maker_position]));
}

const std::vector<TLimitVector>& Scanner::GetAsk() const {
    return ask_;
}

const std::vector<TLimitVector>& Scanner::GetBid() const {
    return bid_;
}

const std::vector<CompletedTransaction>& Scanner::GetTransactions() const {
    return transactions_;
}