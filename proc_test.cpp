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
    transactions.push_back({"AAPL", 100.0, 10});
    transactions.push_back({"AAPL", 110.0, 20});
    transactions.push_back({"AAPL", 120.0, 30});
    std::pair<int, double> result = calculateVWAP(transactions);
    assert(result.first == 60);
    assert(std::abs(result.second - 113.3333) < 0.0001);
    std::cout << "testCalculateVWAP passed" << std::endl;
}

void testCalculateHighLow() {
    std::vector<Transaction> transactions;
    transactions.push_back({"AAPL", 100.0, 10});
    transactions.push_back({"AAPL", 110.0, 20});
    transactions.push_back({"AAPL", 120.0, 30});
    std::pair<double, double> result = calculateHighLow(transactions);
    assert(result.first == 100.0);
    assert(result.second == 120.0);
    std::cout << "testCalculateHighLow passed" << std::endl;
}

// Integration Tests

void testProcessMorningAuction() {
    mOrders.clear();
    mTransactions.clear();
    openPrice.clear();

    Instrument instrument;
    instrument.instrumentId = "AAPL";
    instrument.currency = "USD";
    instrument.lotSize = 1;
    mInstruments.push_back(instrument);
    instruments["AAPL"] = "USD";
    instrumentsLotSize["AAPL"] = 1;

    Client client1;
    client1.clientId = "C1";
    client1.currencies = {"USD"};
    clients["C1"] = client1;

    Client client2;
    client2.clientId = "C2";
    client2.currencies = {"USD"};
    clients["C2"] = client2;

    mOrders.push_back(addOrder("C1", "AAPL", 100, 150, 9, 30, 0));
    mOrders.push_back(addOrder("C2", "AAPL", 200, 160, 9, 30, 0));
    mOrders.push_back(addOrder("C1", "AAPL", 150, 155, 9, 30, 0));

    int i = processMorningAuction();

    assert(openPrice["AAPL"] == 155.0);
    assert(mTransactions.size() == 2);
    assert(clients["C1"].position["AAPL"] == 50);
    assert(clients["C2"].position["AAPL"] == -50);

    std::cout << "testProcessMorningAuction passed" << std::endl;
}

void testProcessRealTimeTransactions() {
    mOrders.clear();
    mTransactions.clear();

    Instrument instrument;
    instrument.instrumentId = "AAPL";
    instrument.currency = "USD";
    instrument.lotSize = 1;
    mInstruments.push_back(instrument);
    instruments["AAPL"] = "USD";
    instrumentsLotSize["AAPL"] = 1;

    Client client1;
    client1.clientId = "C1";
    client1.currencies = {"USD"};
    clients["C1"] = client1;

    Client client2;
    client2.clientId = "C2";
    client2.currencies = {"USD"};
    clients["C2"] = client2;

    mOrders.push_back(addOrder("C1", "AAPL", 100, 150, 10, 0, 0));
    mOrders.push_back(addOrder("C2", "AAPL", 200, 160, 10, 0, 0));
    mOrders.push_back(addOrder("C1", "AAPL", 150, 155, 10, 0, 0));

    int i = processRealTimeTransactions(0);

    assert(mTransactions.size() == 2);
    assert(clients["C1"].position["AAPL"] == -50);
    assert(clients["C2"].position["AAPL"] == 50);

    std::cout << "testProcessRealTimeTransactions passed" << std::endl;
}

void testProcessEveningAuction() {
    mOrders.clear();
    mTransactions.clear();
    closePrice.clear();

    Instrument instrument;
    instrument.instrumentId = "AAPL";
    instrument.currency = "USD";
    instrument.lotSize = 1;
    mInstruments.push_back(instrument);
    instruments["AAPL"] = "USD";
    instrumentsLotSize["AAPL"] = 1;

    Client client1;
    client1.clientId = "C1";
    client1.currencies = {"USD"};
    client1.position["AAPL"] = 100;
    clients["C1"] = client1;

    Client client2;
    client2.clientId = "C2";
    client2.currencies = {"USD"};
    clients["C2"] = client2;

    mOrders.push_back(addOrder("C1", "AAPL", 100, 150, 16, 0, 0));
    mOrders.push_back(addOrder("C2", "AAPL", 200, 160, 16, 0, 0));
    mOrders.push_back(addOrder("C1", "AAPL", 150, 155, 16, 0, 0));

    int i = processEveningAuction(0);

    assert(closePrice["AAPL"] == 155.0);
    assert(mTransactions.size() == 2);
    assert(clients["C1"].position["AAPL"] == 0);
    assert(clients["C2"].position["AAPL"] == 100);

    std::cout << "testProcessEveningAuction passed" << std::endl;
}

int main() {
    // Unit Tests
    testAddOrder();
    testPerformPositionCheckingSell();
    testAddTransaction();
    testCalculateVWAP();
    testCalculateHighLow();

    // Integration Tests
    testProcessMorningAuction();
    testProcessRealTimeTransactions();
    testProcessEveningAuction();

    return 0;
}