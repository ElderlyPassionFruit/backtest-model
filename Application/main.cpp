#include "../BackTest/backtest_includes.h"

#include <chrono>
#include <iostream>
#include <queue>
#include <random>

std::mt19937 rnd(1791791791);

clock_t start;

long double GetCurrentTime() {
    return (long double)(clock() - start) / CLOCKS_PER_SEC;
}

const std::string path_orderbook = "../Data/orderbooks_eth_depth50.csv";
const std::string path_transactions = "../Data/trades_eth.csv";
const uint64_t initial_time = 1603659600000;
const uint64_t end_time = 1603663200000;
const uint64_t finish_time = 1000;
const uint64_t step = 1000;
const bool convert_into_cash = true;
const uint64_t cancel_open_order_time = 10000;
// all measures multiply by 10^4
const uint64_t limit_order_volume = 10;
const int64_t limit_order_price_step = 100;

struct ForCancel {
    uint64_t cancel_timestamp;
    uint64_t order_id;
    ForCancel() = default;
    ForCancel(const uint64_t& cancel_timestamp, const uint64_t& order_id)
        : cancel_timestamp(cancel_timestamp), order_id(order_id) {
    }
};

enum PREDICTION { BUY, SELL, WAIT };

// just for test, that execution works correctly
PREDICTION GetRandomPrediction(const BackTest& /*backtest*/) {
    static std::uniform_int_distribution<> random_prediction(0, 2);
    uint32_t prediction = random_prediction(rnd);
    if (prediction == 0) {
        return BUY;
    } else if (prediction == 1) {
        return SELL;
    } else if (prediction == 2) {
        return WAIT;
    } else {
        throw std::runtime_error("GetRandomPrediction - Random works incorrect.");
    }
}

PREDICTION GetCountTransactionsPredictoin(const BackTest& backtest) {
    static const uint64_t butch = 10;
    auto& transactions = backtest.GetCompletedTrades();
    if (transactions.size() < butch) {
        return GetRandomPrediction(backtest);
    } else {
        int balance = 0;
        for (uint64_t i = 0; i < butch; ++i) {
            if (transactions[transactions.size() - 1 - i]->GetIsBuyerMaker()) {
                ++balance;
            } else {
                --balance;
            }
        }
        if (abs(balance) < butch / 2) {
            return WAIT;
        } else if (balance > 0) {
            return BUY;
        } else {
            return SELL;
        }
    }
}

PREDICTION GetVolumeTransactionsPredictoin(const BackTest& backtest) {
    static const uint64_t butch = 15;
    auto& transactions = backtest.GetCompletedTrades();
    if (transactions.size() < butch) {
        return GetRandomPrediction(backtest);
    } else {
        uint64_t total = 0;
        int64_t balance = 0;
        for (uint64_t i = 0; i < butch; ++i) {
            auto& transaction = transactions[transactions.size() - 1 - i];
            total += transaction->GetVolume();
            if (transaction->GetIsBuyerMaker()) {
                balance += transaction->GetVolume();
            } else {
                balance -= transaction->GetVolume();
            }
        }
        if (abs(balance) < total / 2) {
            return WAIT;
        } else if (balance > 0) {
            return BUY;
        } else {
            return SELL;
        }
    }
}

uint64_t GetAverageCost(const BackTest& backtest, const uint64_t& n) {
    uint64_t total_cost = 0;
    uint64_t total_volume = 0;
    auto& transactions = backtest.GetCompletedTrades();
    for (uint64_t i = 0; i < n; ++i) {
        auto& transaction = transactions[transactions.size() - 1 - i];
        total_cost += transaction->GetVolume() * transaction->GetPrice();
        total_volume += transaction->GetVolume();
    }
    // I decided to ignore the margin of error here
    return total_cost / total_volume;
}

PREDICTION GetAgeragePrediction(const BackTest& backtest) {
    static const uint64_t butch = 10;
    auto& transactions = backtest.GetCompletedTrades();
    if (transactions.size() < butch) {
        return GetRandomPrediction(backtest);
    } else {
        auto total_avg = GetAverageCost(backtest, butch);
        auto cur_avg = GetAverageCost(backtest, butch / 5);
        static const uint64_t interesting_difference = 200;
        if (abs(total_avg - cur_avg) < interesting_difference) {
            return WAIT;
        } else if (cur_avg > total_avg) {
            return BUY;
        } else {
            return SELL;
        }
    }
}

PREDICTION GetMarketPressurePrediction(const BackTest& backtest) {
    int64_t diff = 0;
    for (const auto& order : backtest.GetAsk()) {
        diff += order->GetVolume();
    }
    for (const auto& order : backtest.GetBid()) {
        diff -= order->GetVolume();
    }
    if (abs(diff * 10) < backtest.GetTotalMarketAsset()) {
        return WAIT;
    } else if (diff > 0) {
        return SELL;
    } else {
        return BUY;
    }
}

PREDICTION GetTopPressurePrediction(const BackTest& backtest) {
    const uint64_t cnt = 10;
    int64_t diff = 0;
    uint64_t sum = 0;
    uint64_t cur = 0;
    for (const auto& order : backtest.GetAsk()) {
        diff += order->GetVolume();
        sum += order->GetVolume();
        ++cur;
        if (cur == cnt) {
            break;
        }
    }
    cur = 0;
    for (const auto& order : backtest.GetBid()) {
        diff -= order->GetVolume();
        sum += order->GetVolume();
        ++cur;
        if (cur == cnt) {
            break;
        }
    }
    if (abs(diff * 8) < backtest.GetTotalMarketAsset()) {
        return WAIT;
    } else if (diff > 0) {
        return SELL;
    } else {
        return BUY;
    }
}

PREDICTION GetMixedPrediction(const BackTest& backtest) {
    std::array<int, 3> count = {0, 0, 0};
    ++count[GetCountTransactionsPredictoin(backtest)];
    ++count[GetVolumeTransactionsPredictoin(backtest)];
    // ++count[GetAgeragePrediction(backtest)];
    ++count[GetMarketPressurePrediction(backtest)];
    ++count[GetTopPressurePrediction(backtest)];
    for (const auto& prediction : {BUY, SELL}) {
        if (count[prediction] == 4) {
            return prediction;
        }
    }
    return WAIT;
}

PREDICTION GetPrediction(const BackTest& backtest) {
    // return GetRandomPrediction(backtest);
    // return GetCountTransactionsPredictoin(backtest);
    // return GetVolumeTransactionsPredictoin(backtest);
    // return GetAgeragePrediction(backtest);
    // return GetMarketPressurePrediction(backtest);
    // return GetTopPressurePrediction(backtest);
    return GetMixedPrediction(backtest);
}

void WithdrawAllOrders(BackTest& backtest) {
    for (const auto& order : backtest.GetUserLimitAsk()) {
        if (!order->IsClosed() && !order->IsCanceled()) {
            while (!backtest.WithdrawLimitOrder(order->GetOrderId())) {
                backtest.ProcessTimeInterval(backtest.GetCallFrequency());
            }
        }
    }
    for (const auto& order : backtest.GetUserLimitBid()) {
        if (!order->IsClosed() && !order->IsCanceled()) {
            while (!backtest.WithdrawLimitOrder(order->GetOrderId())) {
                backtest.ProcessTimeInterval(backtest.GetCallFrequency());
            }
        }
    }
}

void SendFinishMarketOrder(BackTest& backtest, int64_t& cash, int64_t& asset) {
    if (asset > 0) {
        auto id = backtest.SendMarketOrder(ASK, asset);
        if (!id) {
            throw std::runtime_error(
                "SendFinishMarketOrder - There should be no problems with sending an ASK market "
                "order.");
        }
        backtest.ProcessTimeInterval(backtest.GetCallFrequency());
    } else if (asset < 0) {
        auto id = backtest.SendMarketOrder(BID, -asset);
        if (!id) {
            throw std::runtime_error(
                "SendFinishMarketOrder - There should be no problems with sending a BID market "
                "order.");
        }
        backtest.ProcessTimeInterval(backtest.GetCallFrequency());
    }
    auto PNL = backtest.GetPNL();
    cash = PNL.total_cash;
    asset = PNL.total_asset;
}

bool ExecuteCancelQueue(BackTest& backtest, std::queue<ForCancel>& cancel_queue) {
    bool ok = false;
    backtest.ProcessBeforeUnlock();
    while (!cancel_queue.empty() &&
           cancel_queue.front().cancel_timestamp >= backtest.GetCurrentTimestamp()) {
        if (!backtest.WithdrawLimitOrder(cancel_queue.front().order_id)) {
            throw std::runtime_error(
                "SendFinishMarketOrder - There have to be no problems with canceling an order.");
        }
        cancel_queue.pop();
        backtest.ProcessBeforeUnlock();
        ok = true;
    }
    return ok;
}

void Execution() {
    start = clock();
    BackTest backtest(path_orderbook, path_transactions);
    std::queue<ForCancel> cancel_queue;
    backtest.ProcessTimeInterval(initial_time);

    std::cerr << "Total cash = " << backtest.GetTotalMarketCash()
              << ", Total asset = " << backtest.GetTotalMarketAsset() << std::endl;
    std::cerr << "Init backtest. time: " << GetCurrentTime() << std::endl;

    int64_t cash = 0, asset = 0;
    uint64_t total_orders = 0;
    while (backtest.GetCurrentTimestamp() < end_time) {
        if (ExecuteCancelQueue(backtest, cancel_queue)) {
            continue;
        }
        backtest.ProcessBeforeUnlock();
        auto prediction = GetPrediction(backtest);

        if (prediction != WAIT) {
            auto cur_price = (backtest.GetBestBid() + backtest.GetBestAsk()) / 2 +
                             (prediction == BUY ? -limit_order_price_step : limit_order_price_step);
            // std::cerr << "cur_price = " << cur_price << std::endl;
            auto id = backtest.SendLimitOrder(prediction == BUY ? BID : ASK, limit_order_volume,
                                              cur_price);
            if (!id) {
                throw std::runtime_error(
                    "Execution - There have to be no problems with sendin a limit order.");
            }
            cancel_queue.push(
                ForCancel(backtest.GetCurrentTimestamp() + cancel_open_order_time, id.value()));
            ++total_orders;
        }
        backtest.ProcessTimeInterval(step);
    }
    ExecuteCancelQueue(backtest, cancel_queue);
    backtest.ProcessTimeInterval(std::max(backtest.GetCancelLatency(), backtest.GetPostLatency()));
    if (backtest.GetCurrentTimestamp() < end_time + finish_time) {
        backtest.ProcessTimeInterval(end_time + finish_time - backtest.GetCurrentTimestamp());
    }
    auto PNL = backtest.GetPNL();
    PNL.Print();
    cash = PNL.total_cash;
    asset = PNL.total_asset;
    WithdrawAllOrders(backtest);
    if (convert_into_cash) {
        SendFinishMarketOrder(backtest, cash, asset);
    }

    // backtest.PrintOrderBook();
    std::cerr << "Total number of orders = " << total_orders << std::endl;
    std::cerr << "Final amount of cash = " << cash << ", final amount of asset = " << asset
              << std::endl;
    std::cerr.precision(4);
    std::cerr << "Average profit from the transaction = " << cash / total_orders << std::endl;
    std::cerr << "Finish testing. time: " << GetCurrentTime() << std::endl;
}

int main() {
    try {
        Execution();
    } catch (const std::exception& e) {
        std::cerr << "Failed with an exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}