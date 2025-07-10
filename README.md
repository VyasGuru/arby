# Arby - Currency Arbitrage Detection System

A high-performance currency arbitrage detection system that identifies profitable trading opportunities across multiple currency pairs using real-time exchange rates.

## Features

- **Multi-threaded Processing**: Parallel analysis of currency chunks for faster detection
- **Real-time Rate Fetching**: Live exchange rates from ExchangeRate-API
- **Bellman-Ford Algorithm**: Efficient cycle detection for arbitrage opportunities
- **Dual Implementation**: Both C++ and Python versions available
- **Configurable Parameters**: Adjustable cycle lengths, thresholds, and chunk sizes
- **Performance Monitoring**: Detailed timing and logging of detection processes

## Project Structure

```
arby/
├── cppversion/           # C++ implementation
│   ├── main.cpp         # Main C++ entry point
│   ├── fetcher.cpp      # API rate fetching
│   ├── fetcher.hpp      # Fetcher header
│   ├── graph.cpp        # Graph algorithms
│   ├── graph.hpp        # Graph header
│   └── json.hpp         # JSON parsing library
├── binarby.py           # Python implementation
├── requirements.txt     # Python dependencies
├── arbitrage.log        # Execution logs
└── arbitrage_log.csv    # Results data
```

##  Setup

### Prerequisites

- **C++ Version**: GCC with C++17 support
- **Python Version**: Python 3.8+ with pip

### Installation

1. **Clone the repository**:
   ```bash
   git clone <repository-url>
   cd arby
   ```

2. **For Python version**:
   ```bash
   # Create virtual environment
   python -m venv venv
   source venv/bin/activate  # On Windows: venv\Scripts\activate
   
   # Install dependencies
   pip install -r requirements.txt
   ```

3. **For C++ version**:
   ```bash
   cd cppversion
   g++ -std=c++17 main.cpp fetcher.cpp graph.cpp -o arby -lcurl
   ```

### API Configuration

The system uses the ExchangeRate-API for real-time exchange rates. To use real API calls:

1. Get a free API key from [ExchangeRate-API](https://www.exchangerate-api.com/)
2. Update the `API_KEY` variable in both implementations:
   - `cppversion/main.cpp` 
   - `binarby.py` 

**Note**: The current implementation has a blank API key to prevent real API usage during development.

##  Usage

### Python Version

```bash
# Activate virtual environment
source venv/bin/activate

# Run the arbitrage detector
python binarby.py
```

### C++ Version

```bash
cd cppversion
./arby
```



### C++ Implementation
- **Multi-threading**: Uses `std::thread` for parallel chunk processing
- **Graph Algorithms**: Custom implementation of Bellman-Ford for cycle detection
- **Memory Efficient**: Optimized data structures for large currency matrices

### Python Implementation
- **Multiprocessing**: Uses `multiprocessing.Pool` for parallel execution
- **Pandas Integration**: Efficient DataFrame operations for rate calculations
- **ThreadPoolExecutor**: Concurrent API rate fetching

## Performance

- **Rate Fetching**: ~sub 1 second for upto 40 currencies
- **Cycle Detection**: Parallel processing reduces analysis time by ~80%
- **Memory Usage**: Optimized for handling large currency matrices
- **Accuracy**: Detects arbitrage opportunities with configurable profit thresholds






