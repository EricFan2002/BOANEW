#include <iostream>
#include "csv.hpp"
#include <queue>
#include <float.h>
#include <vector>
#include <map>
#include <set>
#include <fstream>
using namespace std;

vector<Client> mClients;
map<string, Client> clients;
vector<Instrument> mInstruments;
vector<Order> mOrders;
vector<Transaction> mTransactions;
map<string, string> instruments;
map<string, int> instrumentsLotSize;
vector<pair<string, string>> exchangeReport;
map<string, double> openPrice;
map<string, double> closePrice;

struct SellComparatorRealTime
{
    bool operator()(const Order *a, const Order *b)
    {
        // sell: low price, client rating
        if (a->price != b->price)
            return a->price > b->price; // Prioritize lower price for sell orders
        else
            return clients[a->client].rating > clients[b->client].rating; // Prioritize higher user rating for equal prices
    }
};

struct BuyComparatorRealTime
{
    bool operator()(const Order *a, const Order *b)
    {
        if (a->price != b->price)
            return a->price < b->price; // Prioritize higher price for sell orders
        else
            return clients[a->client].rating > clients[b->client].rating; // Prioritize higher user rating for equal prices
    }
};

struct Time
{
    int hr;
    int min;
    int sec;
    bool operator<(const Time &other) const
    {
        if (hr != other.hr)
            return hr < other.hr;
        if (min != other.min)
            return min < other.min;
        return sec < other.sec;
    }
};

struct SellComparator
{
    bool operator()(const Order *a, const Order *b)
    {
        if (a->price != b->price)
            return a->price > b->price; // Prioritize lower price for sell orders
        else
            return clients[a->client].rating > clients[b->client].rating; // Prioritize higher user rating for equal prices
    }
};

struct BuyComparator
{
    bool operator()(const Order *a, const Order *b)
    {
        if (a->price != b->price)
            return a->price > b->price; // Prioritize higher price for sell orders
        else
            return clients[a->client].rating > clients[b->client].rating; // Prioritize higher user rating for equal prices
    }
};

map<string, priority_queue<Order *, vector<Order *>, SellComparator>> sellOrders;
map<string, priority_queue<Order *, vector<Order *>, BuyComparator>> buyOrders;

map<string, priority_queue<Order *, vector<Order *>, SellComparatorRealTime>> sellOrdersRealTime;
map<string, priority_queue<Order *, vector<Order *>, BuyComparatorRealTime>> buyOrdersRealTime;

struct OrderAccumulate
{
    int shares;
    double price;
    string client;
    string stock;
    Order *order;
};

map<string, vector<OrderAccumulate>> sellOrdersList;
map<string, vector<OrderAccumulate>> buyOrdersList;

int sellOrderCnt = 0;
int buyOrderCnt = 0;
int sellOrderAccumulate = 0;
int buyOrderAccumulate = 0;

int performPositionCheckingSell(string client, string stock, int amount) // return how much can be bought
{
    if (clients[client].positionCheck == 1)
    {
        if (clients[client].position.count(stock))
        {
            return clients[client].position[stock];
        }
        else
            return 0;
    }
    else
        return amount;
}

Order addOrder(string client, string instrument, int quantity, int price, int hours, int mins, int sec)
{
    Order o;
    o.instrument = instrument;
    o.client = client;
    o.hours = hours;
    o.mins = mins;
    o.secs = sec;
    o.price = price;
    o.quantity = quantity;
    return o;
}

map<string, double> getBestPrice()
{
    map<string, double> res;
    for (auto ins : mInstruments)
    {
        int maxSell = 0;
        double maxAmount = 0;
        string thisins = ins.instrumentId;
        sellOrderCnt = 0;
        buyOrderCnt = 0;
        buyOrderAccumulate = 0;
        sellOrderAccumulate = 0;
        sellOrdersList[thisins].reserve(sellOrders[thisins].size());
        buyOrdersList[thisins].reserve(buyOrders[thisins].size());
        while (!sellOrders[thisins].empty())
        {
            auto order = sellOrders[thisins].top();
            sellOrders[thisins].pop();
            if (clients[order->client].positionCheck)
            {
                if (clients[order->client].position.count(order->instrument))
                {
                    if (clients[order->client].position[order->instrument] < order->quantity)
                        continue;
                }
                else
                    continue;
            }
            sellOrdersList[thisins][sellOrderCnt].price = order->price;
            sellOrdersList[thisins][sellOrderCnt].client = order->client;
            sellOrdersList[thisins][sellOrderCnt].stock = order->instrument;
            sellOrdersList[thisins][sellOrderCnt].shares = order->quantity;
            sellOrderCnt++;
            cout << "SELL " << order->price << " at " << order->time << endl;
        }
        while (!buyOrders[thisins].empty())
        {
            auto order = buyOrders[thisins].top();
            buyOrders[thisins].pop();
            buyOrdersList[thisins][buyOrderCnt].price = order->price;
            buyOrdersList[thisins][buyOrderCnt].client = order->client;
            buyOrdersList[thisins][buyOrderCnt].stock = order->instrument;
            buyOrdersList[thisins][buyOrderCnt].shares = order->quantity;
            buyOrderAccumulate += order->quantity;
            buyOrderCnt++;
            cout << "BUY " << order->price << " at " << order->time << endl;
        }
        int sellI = 0;
        for (int i = 0; i < buyOrderCnt; i++)
        {
            auto buy = buyOrdersList[thisins][i];
            double price = buy.price;
            double maxSellPrice = 0;
            // cout << "sell min price " << sellOrdersList[thisins][sellI].price << endl;
            // cout << (sellOrdersList[thisins][sellI].price <= buy.price) << " " << (sellI < sellOrderCnt) << " " << sellOrderCnt << endl;
            while (sellOrdersList[thisins][sellI].price <= buy.price && sellI < sellOrderCnt)
            {
                sellOrderAccumulate += sellOrdersList[thisins][sellI].shares;
                maxSellPrice = maxSellPrice > sellOrdersList[thisins][sellI].price ? maxSellPrice : sellOrdersList[thisins][sellI].price;
                sellI++;
            }
            if (buy.price == DBL_MAX)
                price = maxSellPrice;
            cout << "@ " << price << " can buy " << buyOrderAccumulate << " can sell " << sellOrderAccumulate << endl;
            int sell = min(buyOrderAccumulate, sellOrderAccumulate);
            if (sell >= maxSell)
            {
                maxSell = sell;
                maxAmount = price;
            }
            buyOrderAccumulate -= buy.shares;
        }
        cout << thisins << " : " << maxAmount << endl;
        res[thisins] = maxAmount;
    }
    return res;
}

void addTransaction(string from, string to, string stock, int amount, double price, int hr, int min, int sec)
{
    Transaction t;
    t.from = from;
    t.to = to;
    t.amount = amount;
    t.hours = hr;
    t.mins = min;
    t.secs = sec;
    t.price = price;
    t.stock = stock;
    cout << "-------------------\nTransaction Details:" << endl;
    cout << "From: " << t.from;
    cout << "  To: " << t.to;
    cout << "  Stock: " << t.stock;
    cout << "  Amount: " << t.amount;
    cout << "  Price: " << t.price;
    cout << "  Time: " << t.hours << ":" << t.mins << ":" << t.secs << endl;
    clients[to].position[stock] += amount;
    clients[from].position[stock] -= amount;
    mTransactions.push_back(t);
}

void processAllMorningTransactions(map<string, double> targetPrice)
{
    map<string, double> res;
    for (auto ins : mInstruments)
    {
        int maxSell = 0;
        double maxAmount = 0;
        string thisins = ins.instrumentId;
        sellOrderCnt = 0;
        buyOrderCnt = 0;
        buyOrderAccumulate = 0;
        sellOrderAccumulate = 0;
        sellOrdersList[thisins].reserve(sellOrders[thisins].size());
        buyOrdersList[thisins].reserve(buyOrders[thisins].size());
        while (!sellOrders[thisins].empty())
        {
            auto order = sellOrders[thisins].top();
            sellOrders[thisins].pop();
            if (order->price != 0)
            {
                if (order->price > targetPrice[thisins])
                {
                    sellOrdersRealTime[thisins].push(order);
                }
                else
                {
                    if (clients[order->client].positionCheck)
                    {
                        continue;
                    }
                    sellOrdersList[thisins][sellOrderCnt].price = order->price;
                    sellOrdersList[thisins][sellOrderCnt].client = order->client;
                    sellOrdersList[thisins][sellOrderCnt].stock = order->instrument;
                    sellOrdersList[thisins][sellOrderCnt].shares = order->quantity;
                    sellOrdersList[thisins][sellOrderCnt].order = order;
                    sellOrderCnt++;
                    cout << "+SELL " << order->client << " " << order->price << " " << order->quantity << " at " << order->time << endl;
                }
            }
        }
        while (!sellOrders[thisins].empty())
        {
            auto order = sellOrders[thisins].top();
            sellOrders[thisins].pop();
            if (order->price == 0)
            {
                if (order->price > targetPrice[thisins])
                    break;
                if (clients[order->client].positionCheck)
                {
                    continue;
                }
                sellOrdersList[thisins][sellOrderCnt].price = order->price;
                sellOrdersList[thisins][sellOrderCnt].client = order->client;
                sellOrdersList[thisins][sellOrderCnt].stock = order->instrument;
                sellOrdersList[thisins][sellOrderCnt].shares = order->quantity;
                sellOrdersList[thisins][sellOrderCnt].order = order;
                sellOrderCnt++;
                cout << "+SELL " << order->client << " " << order->price << " " << order->quantity << " at " << order->time << endl;
            }
            else
                break;
        }
        while (!buyOrders[thisins].empty())
        {
            auto order = buyOrders[thisins].top();
            buyOrders[thisins].pop();
            if (order->price > targetPrice[thisins])
            {
                buyOrdersList[thisins][buyOrderCnt].price = order->price;
                buyOrdersList[thisins][buyOrderCnt].client = order->client;
                buyOrdersList[thisins][buyOrderCnt].stock = order->instrument;
                buyOrdersList[thisins][buyOrderCnt].shares = order->quantity;
                buyOrdersList[thisins][buyOrderCnt].order = order;
                buyOrderAccumulate += order->quantity;
                buyOrderCnt++;
                cout << "+BUY " << order->client << " " << order->price << " " << order->quantity << " at " << order->time << endl;
            }
            else
            {
                buyOrdersRealTime[thisins].push(order);
            }
        }
        int sellI = 0;
        int buyI = 0;
        int totalSale = 0;
        while (sellI < sellOrderCnt && buyI < buyOrderCnt)
        {
            auto &buy = buyOrdersList[thisins][buyI];
            auto &sell = sellOrdersList[thisins][sellI];
            int trade = min(buyOrdersList[thisins][buyI].shares, sellOrdersList[thisins][sellI].shares);
            totalSale += trade;
            cout << sell.client << " buy " << buy.client << " buy " << trade << endl;
            buy.shares -= trade;
            sell.shares -= trade;
            double price = targetPrice[thisins];
            cout << " BUY -= " << buy.shares << " SELL -= " << sell.shares << " @ " << price << endl;
            addTransaction(sell.client, buy.client, thisins, trade, price, 9, 30, 0);
            if (buy.shares == 0)
            {
                buyI++;
            }
            else
            {
                sellI++;
            }
        }
        while (sellI < sellOrderCnt)
        {
            auto &sell = sellOrdersList[thisins][sellI];
            Order *o = new Order;
            memcpy(o, sell.order, sizeof(Order));
            o->quantity = sell.shares;
            sellI++;
            sellOrdersRealTime[thisins].push(o);
            cout << "add extra sell " << sell.client << " " << o->quantity << endl;
        }
        while (buyI < buyOrderCnt)
        {
            auto &buy = buyOrdersList[thisins][buyI];
            Order *o = new Order;
            memcpy(o, buy.order, sizeof(Order));
            o->quantity = buy.shares;
            buyI++;
            buyOrdersRealTime[thisins].push(o);
            cout << "add extra buy " << buy.client << " " << o->quantity << endl;
        }
        cout << thisins << " Sale : " << totalSale << endl;
        if (totalSale > 0)
        {
            openPrice[thisins] = targetPrice[thisins];
        }
        res[thisins] = maxAmount;
    }
}

inline int testIfMorning(Order order)
{
    if (order.hours > 9)
        return 0;
    else if (order.hours == 9 && order.mins >= 30)
        return 0;
    return 1;
}

inline int testIfEveningAuctionStart(Order order)
{
    if (order.hours > 16)
        return 1;
    if (order.hours == 16 && order.mins <= 9)
        return 1;
    return 0;
}

inline int testIfEveningAuctionEnd(Order order)
{
    if (order.hours > 16)
        return 1;
    if (order.hours == 16 && order.mins >= 10)
        return 1;
    return 0;
}

int processMorningAuction()
{
    int i;
    for (i = 0; i < mOrders.size(); i++)
    {
        auto &oneOrder = mOrders[i];
        if (!testIfMorning(oneOrder))
            break;
        if (oneOrder.side[0] == 'B')
        {
            cout << "[B] " << oneOrder.time << " " << oneOrder.side << " " << oneOrder.price << endl;
            buyOrders[oneOrder.instrument].push(&oneOrder);
        }
        else if (oneOrder.side[0] == 'S')
        {
            cout << "[S] " << oneOrder.time << " " << oneOrder.side << " " << oneOrder.price << endl;
            sellOrders[oneOrder.instrument].push(&oneOrder);
        }
        else
        {
            cout << "Error " << oneOrder.orderId << " Type >" << oneOrder.side << "<" << endl;
        }
    }
    auto res = getBestPrice();
    buyOrders.clear();
    sellOrders.clear();
    for (int i = 0; i < mOrders.size(); i++)
    {
        auto &oneOrder = mOrders[i];
        if (!testIfMorning(oneOrder))
            break;
        if (oneOrder.side[0] == 'B')
        {
            cout << "[B] " << oneOrder.time << " " << oneOrder.side << " " << oneOrder.price << endl;
            buyOrders[oneOrder.instrument].push(&oneOrder);
        }
        else if (oneOrder.side[0] == 'S')
        {
            cout << "[S] " << oneOrder.time << " " << oneOrder.side << " " << oneOrder.price << endl;
            sellOrders[oneOrder.instrument].push(&oneOrder);
        }
        else
        {
            cout << "Error " << oneOrder.orderId << " Type >" << oneOrder.side << "<" << endl;
        }
    }
    processAllMorningTransactions(res);
    // for (auto i : mInstruments)
    // {
    //     string name = i.instrumentId;
    //     while (!buyOrdersRealTime[name].empty())
    //     {
    //         auto &i = buyOrdersRealTime[name].top();
    //         cout << "REALTIME BUY " << i->client << " pri " << i->price << " amout " << i->quantity << endl;
    //         buyOrdersRealTime[name].pop();
    //     }
    // }
    // for (auto i : mInstruments)
    // {
    //     string name = i.instrumentId;
    //     while (!sellOrdersRealTime[name].empty())
    //     {
    //         auto &i = sellOrdersRealTime[name].top();
    //         cout << "REALTIME SELL " << i->client << " pri " << i->price << " amout " << i->quantity << endl;
    //         sellOrdersRealTime[name].pop();
    //     }
    // }
    return i;
}

int processRealTimeTransactions(int start)
{
    while (start < mOrders.size())
    {
        // cout << "@" << start << endl;
        int if_any = 0;
        for (auto ins : mInstruments)
        {
            string thisins = ins.instrumentId;
            if (sellOrdersRealTime[thisins].empty() || buyOrdersRealTime[thisins].empty())
            {
                // cout << thisins << " no order!" << endl;
                continue;
            }
            else
            {
                if (sellOrdersRealTime[thisins].top()->price <= buyOrdersRealTime[thisins].top()->price)
                {
                    int max_amount = min(sellOrdersRealTime[thisins].top()->quantity, buyOrdersRealTime[thisins].top()->quantity);
                    auto &sell = sellOrdersRealTime[thisins].top();
                    auto &buy = buyOrdersRealTime[thisins].top();
                    cout << "sell " << sell->price << " buy " << buy->price << endl;
                    sell->quantity -= max_amount;
                    buy->quantity -= max_amount;
                    Time time1{sell->hours, sell->mins, sell->secs};
                    Time time2{buy->hours, buy->mins, buy->secs};
                    cout << sell->hours << " " << sell->mins << " " << sell->secs << " vs ";
                    cout << buy->hours << " " << buy->mins << " " << buy->secs << " USE ";
                    Time maxTime;
                    double fPrice = 0;
                    if (time1 < time2)
                    {
                        fPrice = sell->price;
                        maxTime = time2;
                        cout << "2 buy" << endl;
                    }
                    else
                    {
                        fPrice = buy->price;
                        maxTime = time1;
                        cout << "1 sell" << endl;
                    }
                    addTransaction(sell->client, buy->client, thisins, max_amount, fPrice, maxTime.hr, maxTime.min, maxTime.sec);
                    if_any = 1;
                    if (sellOrdersRealTime[thisins].top()->quantity == 0)
                    {
                        sellOrdersRealTime[thisins].pop();
                    }
                    if (buyOrdersRealTime[thisins].top()->quantity == 0)
                    {
                        buyOrdersRealTime[thisins].pop();
                    }
                }
            }
        }
        if (!if_any)
        {
            if (testIfEveningAuctionStart(mOrders[start]))
                break;
            auto &o = mOrders[start];
            cout << "[Add Order] " << start << " @ " << o.hours << " : " << o.mins << " " << o.quantity << " @ " << o.price << endl;
            if (o.side[0] == 'S')
            {
                if (clients[o.client].positionCheck)
                {
                    if (clients[o.client].position[o.instrument] < o.quantity)
                    {
                        // failed
                        o.valid = 0;
                        cout << "Order " << o.client << " " << o.hours << " : " << o.mins << " failed position checking " << endl;
                        exchangeReport.push_back(make_pair(o.orderId, "REJECTED-POSITION CHECK FAILED"));
                        start++;
                        continue;
                    }
                }
                sellOrdersRealTime[o.instrument].push(&o);
            }
            else
            {
                buyOrdersRealTime[o.instrument].push(&o);
            }
            start++;
        }
    }
    return start;
}

void processAllEveningTransactions(map<string, double> targetPrice)
{
    map<string, double> res;
    for (auto ins : mInstruments)
    {
        int maxSell = 0;
        double maxAmount = 0;
        string thisins = ins.instrumentId;
        sellOrderCnt = 0;
        buyOrderCnt = 0;
        buyOrderAccumulate = 0;
        sellOrderAccumulate = 0;
        sellOrdersList[thisins].reserve(sellOrders[thisins].size());
        buyOrdersList[thisins].reserve(buyOrders[thisins].size());
        while (!sellOrders[thisins].empty())
        {
            auto order = sellOrders[thisins].top();
            sellOrders[thisins].pop();
            if (order->price != 0)
            {
                if (clients[order->client].position[order->instrument] < order->quantity)
                    continue;
                if (order->price > targetPrice[thisins])
                {
                    sellOrdersRealTime[thisins].push(order);
                }
                else
                {
                    sellOrdersList[thisins][sellOrderCnt].price = order->price;
                    sellOrdersList[thisins][sellOrderCnt].client = order->client;
                    sellOrdersList[thisins][sellOrderCnt].stock = order->instrument;
                    sellOrdersList[thisins][sellOrderCnt].shares = order->quantity;
                    sellOrdersList[thisins][sellOrderCnt].order = order;
                    sellOrderCnt++;
                    cout << "+SELL " << order->client << " " << order->price << " " << order->quantity << " at " << order->time << endl;
                }
            }
        }
        while (!sellOrders[thisins].empty())
        {
            auto order = sellOrders[thisins].top();
            sellOrders[thisins].pop();
            if (order->price == 0)
            {
                if (clients[order->client].position[order->instrument] < order->quantity)
                    continue;
                if (order->price > targetPrice[thisins])
                    break;
                sellOrdersList[thisins][sellOrderCnt].price = order->price;
                sellOrdersList[thisins][sellOrderCnt].client = order->client;
                sellOrdersList[thisins][sellOrderCnt].stock = order->instrument;
                sellOrdersList[thisins][sellOrderCnt].shares = order->quantity;
                sellOrdersList[thisins][sellOrderCnt].order = order;
                sellOrderCnt++;
                cout << "+SELL " << order->client << " " << order->price << " " << order->quantity << " at " << order->time << endl;
            }
            else
                break;
        }
        while (!buyOrders[thisins].empty())
        {
            auto order = buyOrders[thisins].top();
            buyOrders[thisins].pop();
            if (order->price > targetPrice[thisins])
            {
                buyOrdersList[thisins][buyOrderCnt].price = order->price;
                buyOrdersList[thisins][buyOrderCnt].client = order->client;
                buyOrdersList[thisins][buyOrderCnt].stock = order->instrument;
                buyOrdersList[thisins][buyOrderCnt].shares = order->quantity;
                buyOrdersList[thisins][buyOrderCnt].order = order;
                buyOrderAccumulate += order->quantity;
                buyOrderCnt++;
                cout << "+BUY " << order->client << " " << order->price << " " << order->quantity << " at " << order->time << endl;
            }
            else
            {
                buyOrdersRealTime[thisins].push(order);
            }
        }
        int sellI = 0;
        int buyI = 0;
        int totalSale = 0;
        while (sellI < sellOrderCnt && buyI < buyOrderCnt)
        {
            auto &buy = buyOrdersList[thisins][buyI];
            auto &sell = sellOrdersList[thisins][sellI];
            int trade = min(buyOrdersList[thisins][buyI].shares, sellOrdersList[thisins][sellI].shares);
            totalSale += trade;
            cout << sell.client << " buy " << buy.client << " * " << trade << endl;
            buy.shares -= trade;
            sell.shares -= trade;
            double price = targetPrice[thisins];
            cout << " BUY -= " << buy.shares << " SELL -= " << sell.shares << " @ " << price << endl;
            addTransaction(sell.client, buy.client, thisins, trade, price, 9, 30, 0);
            if (buy.shares == 0)
            {
                buyI++;
            }
            else
            {
                sellI++;
            }
        }
        if (totalSale > 0)
        {
            closePrice[thisins] = targetPrice[thisins];
        }
        cout << thisins << " Sale : " << totalSale << endl;
        res[thisins] = maxAmount;
    }
}

int processEveningAuction(int start)
{
    buyOrders.clear();
    sellOrders.clear();
    for (auto i : mInstruments)
    {
        string thisins = i.instrumentId;
        while (!buyOrdersRealTime[thisins].empty())
        {
            auto &i = buyOrdersRealTime[thisins].top();
            buyOrders[thisins].push(i);
            cout << "[Evening B1] " << i->client << " " << i->hours << " : " << i->mins << " " << i->price << " " << i->quantity << endl;
            buyOrdersRealTime[thisins].pop();
        }
        while (!sellOrdersRealTime[thisins].empty())
        {
            auto &i = sellOrdersRealTime[thisins].top();
            sellOrders[thisins].push(i);
            cout << "[Evening S1] " << i->client << " " << i->hours << " : " << i->mins << " " << i->price << " " << i->quantity << endl;
            sellOrdersRealTime[thisins].pop();
        }
    }
    for (; start < mOrders.size(); start++)
    {
        auto &oneOrder = mOrders[start];
        if (testIfEveningAuctionEnd(oneOrder))
            break;
        if (oneOrder.side[0] == 'B')
        {
            cout << "[Evening B] " << oneOrder.time << " " << oneOrder.quantity << " " << oneOrder.price << endl;
            buyOrders[oneOrder.instrument].push(&oneOrder);
        }
        else if (oneOrder.side[0] == 'S')
        {
            cout << "[Evening S] " << oneOrder.time << " " << oneOrder.quantity << " " << oneOrder.price << endl;
            if (clients[oneOrder.client].position[oneOrder.instrument] < oneOrder.quantity)
                continue;
            sellOrders[oneOrder.instrument].push(&oneOrder);
        }
        else
        {
            cout << "Evening Error " << oneOrder.orderId << " Type >" << oneOrder.side << "<" << endl;
        }
    }
    auto res = getBestPrice();
    buyOrders.clear();
    sellOrders.clear();
    for (auto i : mInstruments)
    {
        string thisins = i.instrumentId;
        while (!buyOrdersRealTime[thisins].empty())
        {
            auto &i = buyOrdersRealTime[thisins].top();
            buyOrders[thisins].push(i);
            cout << "[Evening B1] " << i->client << " " << i->hours << " : " << i->mins << " " << i->price << " " << i->quantity << endl;
            buyOrdersRealTime[thisins].pop();
        }
        while (!sellOrdersRealTime[thisins].empty())
        {
            auto &i = sellOrdersRealTime[thisins].top();
            sellOrders[thisins].push(i);
            cout << "[Evening S1] " << i->client << " " << i->hours << " : " << i->mins << " " << i->price << " " << i->quantity << endl;
            sellOrdersRealTime[thisins].pop();
        }
    }
    for (; start < mOrders.size(); start++)
    {
        auto &oneOrder = mOrders[start];
        if (testIfEveningAuctionEnd(oneOrder))
            break;
        if (oneOrder.side[0] == 'B')
        {
            cout << "[Evening B] " << oneOrder.time << " " << oneOrder.quantity << " " << oneOrder.price << endl;
            buyOrders[oneOrder.instrument].push(&oneOrder);
        }
        else if (oneOrder.side[0] == 'S')
        {
            if (clients[oneOrder.client].position[oneOrder.instrument] < oneOrder.quantity)
                continue;
            cout << "[Evening S] " << oneOrder.time << " " << oneOrder.quantity << " " << oneOrder.price << endl;
            sellOrders[oneOrder.instrument].push(&oneOrder);
        }
        else
        {
            cout << "Evening Error " << oneOrder.orderId << " Type >" << oneOrder.side << "<" << endl;
        }
    }
    processAllEveningTransactions(res);
    // for (auto i : mInstruments)
    // {
    //     string name = i.instrumentId;
    //     while (!buyOrdersRealTime[name].empty())
    //     {
    //         auto &i = buyOrdersRealTime[name].top();
    //         cout << "REALTIME BUY " << i->client << " pri " << i->price << " amout " << i->quantity << endl;
    //         buyOrdersRealTime[name].pop();
    //     }
    // }
    // for (auto i : mInstruments)
    // {
    //     string name = i.instrumentId;
    //     while (!sellOrdersRealTime[name].empty())
    //     {
    //         auto &i = sellOrdersRealTime[name].top();
    //         cout << "REALTIME SELL " << i->client << " pri " << i->price << " amout " << i->quantity << endl;
    //         sellOrdersRealTime[name].pop();
    //     }
    // }
    return start;
}

int checkCurrency(string user, string ins)
{
    string cur = instruments[ins];
    for (auto &i : clients[user].currencies)
    {
        if (cur[0] == i[0])
            return 1;
    }
    return 0;
}

int checkLot(int amount, string ins)
{
    if (amount % instrumentsLotSize[ins] == 0)
        return 1;
    return 0;
}

vector<Order> filter(vector<Order> &orders)
{
    vector<Order> res;
    for (int i = 0; i < orders.size(); i++)
    {
        auto &o = orders[i];
        if (!instruments.count(o.instrument))
        {
            cout << "Instrument Not Found!" << endl;
            exchangeReport.push_back(make_pair(o.orderId, "REJECTED-INSTRUMENT NOT FOUND"));
        }
        else if (!checkCurrency(o.client, o.instrument))
        {
            cout << "Cur Not Found!" << endl;
            exchangeReport.push_back(make_pair(o.orderId, "REJECTED-MISMATCH CURRENCY"));
        }
        else if (!checkLot(o.quantity, o.instrument))
        {
            cout << "Lot err!" << endl;
            exchangeReport.push_back(make_pair(o.orderId, "REJECTED-INVALID LOT SIZE"));
        }
        else
            res.push_back(o);
    }
    return res;
}

pair<int, double> calculateVWAP(const std::vector<Transaction> &transactions)
{
    double totalValue = 0.0;
    double totalVolume = 0;

    for (const auto &transaction : transactions)
    {
        totalValue += transaction.price * transaction.amount;
        totalVolume += transaction.amount;
        cout << transaction.price << " " << transaction.amount << " = " << transaction.price * transaction.amount << endl;
    }

    if (totalVolume == 0)
    {
        return make_pair(0, 0);
    }

    return make_pair(totalVolume, totalValue / totalVolume);
}

pair<double, double> calculateHighLow(const std::vector<Transaction> &transactions)
{
    double high = 0;
    double low = DBL_MAX;
    for (const auto &transaction : transactions)
    {
        high = max(high, transaction.price);
        low = min(low, transaction.price);
    }

    return make_pair(low, high);
}

void outputFormatting()
{
    ofstream ex("./1 Exchange Report.csv");
    ex << "OrderID,RejectionReason" << endl;
    for (auto &i : exchangeReport)
    {
        ex << i.first << "," << i.second << endl;
    }
    ex.close();
    ofstream cl("./2 Client Report.csv");
    cl << "ClientID,InstrumentID,NetPosition" << endl;
    for (auto &i : clients)
    {
        for (auto &j : i.second.position)
        {
            cl << i.first << "," << j.first << "," << j.second << endl;
        }
    }
    cl.close();
    ofstream insr("./3 Instrument Report.csv");
    map<string, vector<Transaction>> trans;
    for (auto &i : mTransactions)
    {
        trans[i.stock].push_back(i);
    }
    insr << "#,InstrumentID,OpenPrice,ClosePrice,TotalVolume,VWAP,DayHigh,DayLow" << endl;
    int j = 1;
    for (auto i : instruments)
    {
        string name = i.first;
        pair<int, double> res1 = calculateVWAP(trans[name]);
        pair<double, double> res2 = calculateHighLow(trans[name]);
        string op = to_string(openPrice[name]);
        if (openPrice[name] == 0)
            op = "NULL";
        string cp = to_string(openPrice[name]);
        if (closePrice[name] == 0)
            cp = "NULL";
        insr
            << j << "," << name << "," << op << "," << cp << "," << res1.first << "," << res1.second << "," << res2.second << "," << res2.first << endl;
        j++;
    }
    insr.close();
}

int main()
{
    readClientsCSV("input_clients.csv", mClients);
    for (auto i : mClients)
        clients[i.clientId] = i;
    readInstrumentsCSV("input_instruments.csv", mInstruments);
    for (auto i : mInstruments)
        instruments[i.instrumentId] = i.currency, instrumentsLotSize[i.instrumentId] = i.lotSize;
    readOrdersCSV("input_orders.csv", mOrders);
    mOrders = filter(mOrders);
    int i = processMorningAuction();
    i = processRealTimeTransactions(i);
    cout << "------------EVENING AUCTION------------------\n";
    processEveningAuction(i);
    outputFormatting();
}