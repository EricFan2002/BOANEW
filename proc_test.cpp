#include <cassert>
#include <iostream>
#include <vector>
#include <utility>
#include <cmath>
#include "proc.cpp"

// Unit Tests

void testAddOrder() {
    Order order = addOrder("C1", "AAPL", 100, 150, 9, 30, 0);
    assert(order.client == "C1");
    assert(order.instrument == "AAPL");
    assert(order.quantity == 100);
    assert(order.price == 150);
    assert(order.hours == 9);
    assert(order.mins == 30);
    assert(order.secs == 0);
    std::cout << "testAddOrder passed" << std::endl;
}

void testPerformPositionCheckingSell() {
    clients["C1"].positionCheck = 1;
    clients["C1"].position["AAPL"] = 100;
    assert(performPositionCheckingSell("C1", "AAPL", 50) == 100);
    assert(performPositionCheckingSell("C1", "GOOGL", 50) == 0);
    clients["C1"].positionCheck = 0;
    assert(performPositionCheckingSell("C1", "AAPL", 50) == 50);
    assert(performPositionCheckingSell("C1", "GOOGL", 50) == 50);
    std::cout << "testPerformPositionCheckingSell passed" << std::endl;
}

void testAddTransaction() {
    mTransactions.clear();
    addTransaction("C1", "C2", "AAPL", 100, 150.0, 9, 30, 0);
    assert(mTransactions.size() == 1);
    assert(mTransactions[0].from == "C1");
    assert(mTransactions[0].to == "C2");
    assert(mTransactions[0].stock == "AAPL");
    assert(mTransactions[0].amount == 100);
    assert(mTransactions[0].price == 150.0);
    assert(mTransactions[0].hours == 9);
    assert(mTransactions[0].mins == 30);
    assert(mTransactions[0].secs == 0);
    std::cout << "testAddTransaction passed" << std::endl;
}

void testCalculateVWAP() {
    std::vector<Transaction> transactions;
    Transaction t1;
    t1.price = 100.0;
    t1.amount = 10;
    transactions.push_back(t1);

    Transaction t2;
    t2.price = 110.0;
    t2.amount = 20;
    transactions.push_back(t2);

    Transaction t3;
    t3.price = 120.0;
    t3.amount = 30;
    transactions.push_back(t3);

    std::pair<int, double> result = calculateVWAP(transactions);
    assert(result.first == 60);
    assert(std::abs(result.second - 113.3333) < 0.0001);
    std::cout << "testCalculateVWAP passed" << std::endl;
}

void testCalculateHighLow() {
    std::vector<Transaction> transactions;
    Transaction t1;
    t1.price = 100.0;
    t1.amount = 10;
    transactions.push_back(t1);

    Transaction t2;
    t2.price = 110.0;
    t2.amount = 20;
    transactions.push_back(t2);

    Transaction t3;
    t3.price = 120.0;
    t3.amount = 30;
    transactions.push_back(t3);

    std::pair<double, double> result = calculateHighLow(transactions);
    assert(result.first == 100.0);
    assert(result.second == 120.0);
    std::cout << "testCalculateHighLow passed" << std::endl;
}

int main() {
    // Unit Tests
    testAddOrder();
    testPerformPositionCheckingSell();
    testAddTransaction();
    testCalculateVWAP();
    testCalculateHighLow();

    return 0;
}