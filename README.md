# BOANEW

BOANEW is a trading system that matches orders and executes transactions based on specific rules and market conditions. It provides functionality for morning auctions, real-time transactions, and evening auctions.

## Features

- Process client, instrument, and order data from CSV files
- Filter orders based on currency and lot size requirements
- Match buy and sell orders based on price compatibility and execute transactions
- Handle position checks and reject orders that fail position checks
- Generate output files for exchange report, client report, and instrument report

## Getting Started

These instructions will help you set up and run the project on your local machine.

### Prerequisites

- C++ compiler with C++20 support
- Python 3.x
- Pipenv (Python package manager)

### Installation

1. Clone the repository:

git clone https://github.com/your-username/BOA2024.git

2. Change to the project directory:

cd BOANEW

3. Create a virtual environment using Pipenv:

pipenv shell

4. Install the required Python dependencies:

pipenv install

### Usage

1. Prepare the input CSV files:
- `input_clients.csv`: Contains client information
- `input_instruments.csv`: Contains instrument information
- `input_orders.csv`: Contains order information

2. Run the C++ program to process the orders and generate output files:

g++ -std=c++20 proc.cpp csv.cpp -o proc
./proc

3. Run the Python application to visualize the trading dashboard:

python app.py

4. Open a web browser and navigate to `http://localhost:8050` to access the trading dashboard.

### Testing

The project includes unit tests and integration tests to ensure the correctness of the implemented functionality.

To run the tests:

g++ -std=c++20 validator_test.cpp csv.cpp -o proc_test
./validator_test

g++ -std=c++20 proc_test.cpp csv.cpp -o proc_test
./proc_test