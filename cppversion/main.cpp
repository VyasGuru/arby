// main.cpp
#include "fetcher.hpp"
#include "graph.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <mutex>
#include <tuple>
#include <unordered_map>
#include <curl/curl.h>
#include "nlohmann/json.hpp"

using namespace std;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

const vector<string> CURRENCIES = {
    "USD", "EUR", "JPY", "GBP", "CNY", "AUD", "CAD", "CHF", "HKD", "SGD",
    "INR", "RUB", "BRL", "ZAR", "NZD", "SEK", "NOK", "DKK", "PLN", "THB",
    "IDR", "MYR", "PHP", "KRW", "ILS", "CZK", "HUF", "MXN", "TRY", "SAR"
};


const int CHUNKS = 4;
const int CHUNK_SIZE = 16;
const int MIN_CYCLE_LENGTH = 3;
const int MAX_CYCLE_LENGTH = 4;
const double thresholdGain = 0.25;

void process_chunk(const vector<string>& chunk,
                   const unordered_map<string, unordered_map<string, double>>& all_rates,
                   vector<tuple<vector<string>, double, double>>& output,
                   double start_time) {
    Graph g(chunk, all_rates);
    auto cycles = g.find_arbitrage_cycles(MIN_CYCLE_LENGTH, MAX_CYCLE_LENGTH);
    for (const auto& cycle : cycles) {
        double gain = g.evaluate_cycle_gain(cycle);
        if (gain > thresholdGain) {
            double elapsed = chrono::duration<double>(chrono::high_resolution_clock::now().time_since_epoch()).count() - start_time;
            output.emplace_back(cycle, gain, elapsed);
        }
    }
}

unordered_map<string, unordered_map<string, double>>
fetch_all_rates_parallel(const string& api_key, const vector<string>& currencies, int num_threads = 4) {
    unordered_map<string, unordered_map<string, double>> data;
    std::mutex data_mutex;

   
    size_t chunk_size = (currencies.size() + num_threads - 1) / num_threads;
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        size_t start = t * chunk_size;
        size_t end = std::min(start + chunk_size, currencies.size());
        if (start >= end) break;

        threads.emplace_back([&, start, end]() {
            CURL* curl = curl_easy_init();
            if (!curl) return;
            for (size_t i = start; i < end; ++i) {
                const auto& base = currencies[i];
                string url = "https://v6.exchangerate-api.com/v6/" + api_key + "/latest/" + base;
                string response_string;

                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

                CURLcode res = curl_easy_perform(curl);
                if (res != CURLE_OK) {
                    cerr << "Failed to fetch rates for " << base << ": " << curl_easy_strerror(res) << endl;
                    continue;
                }

                unordered_map<string, double> base_rates;
                try {
                    auto json_data = nlohmann::json::parse(response_string);
                    if (json_data.contains("conversion_rates")) {
                        for (const auto& quote : currencies) {
                            if (base == quote) base_rates[quote] = 1.0;
                            else base_rates[quote] = json_data["conversion_rates"].value(quote, NAN);
                        }
                    }
                } catch (const std::exception& e) {
                    cerr << "JSON parse error for " << base << ": " << e.what() << endl;
                }

                {
                    std::lock_guard<std::mutex> lock(data_mutex);
                    data[base] = std::move(base_rates);
                }
            }
            curl_easy_cleanup(curl);
        });
    }

    for (auto& th : threads) th.join();
    return data;
}

int main() {
    string api_key = "";  
    auto start_time_point = chrono::high_resolution_clock::now();
    double start_time = chrono::duration<double>(start_time_point.time_since_epoch()).count();


    auto rates = fetch_all_rates_parallel(api_key, CURRENCIES);


    cout << "\nChecking exchange rate matrix:\n";
    for (const auto& from : CURRENCIES) {
        for (const auto& to : CURRENCIES) {
            if (rates[from].find(to) == rates[from].end() || isnan(rates[from][to])) {
                cout << "Missing rate: " << from << " → " << to << endl;
            }
        }
    }


    vector<thread> threads;
    vector<vector<string>> chunks;
    vector<tuple<vector<string>, double, double>> results;
    mutex result_mutex;

    mt19937 rng(random_device{}());
    for (int i = 0; i < CHUNKS; ++i) {
        vector<string> chunk;
        sample(CURRENCIES.begin(), CURRENCIES.end(), back_inserter(chunk), CHUNK_SIZE, rng);
        chunks.push_back(chunk);
    }

    for (const auto& chunk : chunks) {
        threads.emplace_back([&, chunk]() {
            vector<tuple<vector<string>, double, double>> local_results;
            process_chunk(chunk, rates, local_results, start_time);
            lock_guard<mutex> lock(result_mutex);
            results.insert(results.end(), local_results.begin(), local_results.end());
        });
    }

    for (auto& t : threads) t.join();

    if (!results.empty()) {
        cout << "\n💰 Profitable Arbitrage Cycles:\n";
        for (const auto& [path, gain, t] : results) {
            cout << "Gain: " << gain << " | Path: ";
            for (size_t i = 0; i < path.size(); ++i) {
                cout << path[i];
                if (i + 1 < path.size()) cout << " → ";
            }
            cout << " |  Detected after " << t << " sec\n";
        }
    } else {
        cout << "No arbitrage opportunities found.\n";
    }

    return 0;
}
