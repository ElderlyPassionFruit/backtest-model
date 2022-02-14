#include "../BackTest/backtest_includes.h"

#include <chrono>
#include <iostream>
#include <stdexcept>

long double GetTime() {
    return (long double)clock() / CLOCKS_PER_SEC;
}

bool test_completed_transactions = true;
bool test_orders = true;
bool test_orderbook = true;
bool test_scanner = true;
bool test_backtest = true;

const std::string path_orderbook = "../Data/orderbooks_eth_depth50.csv";
const std::string path_transactions = "../Data/trades_eth.csv";
const uint64_t initial_time = 1603659600000;

// tests for completed transactions

void TestCompletedTransactions() {
    try {
        std::cerr << "Basic test for Print of completed transaction" << std::endl;
        CompletedTransaction completed_transaction(120, 10, 37, true);
        completed_transaction.Print();
    } catch (const std::exception& e) {
        std::cerr << "Failed with an exception: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cerr << std::endl;
}

// tests for orders

void TestOrders() {
    try {
        std::cerr << "Basic tests for Print of Market/Limit orders" << std::endl;
        MarketOrder market_order(0, 126, ASK, 10);
        market_order.Print();
        LimitOrder limit_order(1, 128, BID, 5, 12);
        limit_order.Print(false);
        std::cerr << "Tests for adding completed transactions" << std::endl;
        limit_order.AddTransaction(std::make_shared<CompletedTransaction>(129, 3, 10, false));
        limit_order.Print();
        try {
            limit_order.AddTransaction(std::make_shared<CompletedTransaction>(130, 3, 11, true));
            throw std::logic_error(
                "Added incorrect transaction (with incorrect volume), but code didn't failed.");
        } catch (const std::runtime_error& r) {
        }
        try {
            limit_order.AddTransaction(std::make_shared<CompletedTransaction>(120, 1, 11, false));
            throw std::logic_error(
                "Added incorrect transaction (with incorrect timestamp), but code didn't failed.");
        } catch (const std::runtime_error& r) {
        }
        limit_order.Print();
    } catch (const std::exception& e) {
        std::cerr << "Failed with an exception: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cerr << std::endl;
}

// tests for orderbook

void TestOrderBook() {
    try {
        OrderBook orderbook;
        orderbook.Print();
        std::cerr << std::endl;

        TLimitVector historical_ask = {std::make_shared<LimitOrder>(-1, 100, ASK, 10, 5),
                                       std::make_shared<LimitOrder>(-1, 100, ASK, 15, 10)};
        TLimitVector historical_bid = {std::make_shared<LimitOrder>(-1, 100, BID, 8, 4),
                                       std::make_shared<LimitOrder>(-1, 100, BID, 11, 3)};
        orderbook.UpdateOrderBook(historical_ask, historical_bid);
        orderbook.Print();
        std::cerr << std::endl;

        auto id1 = orderbook.AddNewOrder();
        auto id2 = orderbook.AddNewOrder();
        auto id3 = orderbook.AddNewOrder();
        auto id4 = orderbook.AddNewOrder();

        orderbook.AddUserLimitOrder(id1, 120, ASK, 5, 5);
        std::cerr << "id1 = " << id1 << std::endl;
        orderbook.Print();
        std::cerr << std::endl;

        orderbook.AddUserLimitOrder(id2, 130, BID, 4, 3);
        std::cerr << "id2 = " << id2 << std::endl;
        orderbook.Print();
        auto order = orderbook.GetOrderInfo(id1);
        order->Print();
        std::cerr << std::endl;

        orderbook.CompleteUserMarketOrder(id3, 140, BID, 12);
        std::cerr << "id3 = " << id3 << std::endl;
        orderbook.Print();

        orderbook.CompleteUserMarketOrder(id4, 150, ASK, 10);
        std::cerr << "id4 = " << id4 << std::endl;
        orderbook.Print();
        std::cerr << std::endl;

        CompletedTransaction transaction(160, 10, 2, true);
        orderbook.CompleteMarketTransaction(transaction);
        orderbook.Print();

        std::cerr << "avg price for order id3 = " << orderbook.GetOrderInfo(id3)->GetAveragePrice()
                  << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed with an exception: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cerr << std::endl;
}

// tests for scanner

void TestScanner() {
    try {
        auto before = GetTime();
        Scanner scanner;
        scanner.ReadAll(path_orderbook, path_transactions);
        std::cerr << "time for read: " << GetTime() - before << std::endl;

        before = GetTime();
        OrderBook orderbook;
        for (size_t i = 0; i < scanner.GetAsk().size(); ++i) {
            orderbook.UpdateOrderBook(scanner.GetAsk()[i], scanner.GetBid()[i]);
        }
        std::cerr << "time for update all: " << GetTime() - before << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed with an exception: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}

// tests for backtest

void TestBackTest() {
    try {

        {
            auto before = GetTime();
            BackTest backtest(path_orderbook, path_transactions);
            backtest.ProcessTimeInterval(initial_time);
            std::cerr << "Initialized ok" << std::endl;
            backtest.ProcessTimeInterval(initial_time);
            std::cerr << "Complete all ok" << std::endl;
            std::cerr << "idle testing time: " << GetTime() - before << std::endl;
        }

        {
            BackTest backtest(path_orderbook, path_transactions);
            backtest.ProcessTimeInterval(initial_time);
            // backtest.PrintOrderBook();

            auto id1 = backtest.SendLimitOrder(ASK, 10000, 4075200);
            backtest.ProcessTimeInterval(500);
            static_cast<LimitOrder*>(backtest.GetOrderInfo(id1.value()).get())->Print();

            backtest.GetPNL().Print();

            auto id2 = backtest.SendMarketOrder(BID, 200000);
            backtest.ProcessTimeInterval(500);
            static_cast<LimitOrder*>(backtest.GetOrderInfo(id2.value()).get())->Print();

            backtest.GetPNL().Print();

            backtest.ProcessTimeInterval(initial_time);

            auto id3 = backtest.SendLimitOrder(ASK, 1000, 407520000);
            backtest.ProcessTimeInterval(500);
            backtest.WithdrawLimitOrder(id3.value());
            backtest.ProcessTimeInterval(500);
            static_cast<LimitOrder*>(backtest.GetOrderInfo(id3.value()).get())->Print();

            backtest.GetPNL().Print();
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed with an exception: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cerr << std::endl;
}

int main() {
    if (test_completed_transactions) {
        TestCompletedTransactions();
    }

    if (test_orders) {
        TestOrders();
    }

    if (test_orderbook) {
        TestOrderBook();
    }

    if (test_scanner) {
        TestScanner();
    }

    if (test_backtest) {
        TestBackTest();
    }

    std::cerr << "All tests passed!" << std::endl;
    return EXIT_SUCCESS;
}
