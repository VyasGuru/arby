#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <tuple>
#include <chrono>
#include <cmath>
#include <curl/curl.h>
#include "nlohmann/json.hpp"
using namespace std;

const vector<string> CURRENCIES = {
    "USD", "EUR", "JPY", "GBP", "CNY", "AUD", "CAD", "CHF", "HKD", "SGD",
    "INR", "RUB", "BRL", "ZAR", "NZD", "SEK", "NOK", "DKK", "PLN", "THB",
    "IDR", "MYR", "PHP", "KRW", "ILS", "CZK", "HUF", "MXN", "TRY", "SAR"
};
const int MIN_CYCLE_LENGTH = 5;
const int MAX_CYCLE_LENGTH = 10;
const double thresholdGain = 0.35;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* out) {
    size_t totalSize = size * nmemb;
    out->append((char*)contents, totalSize);
    return totalSize;
}

unordered_map<string, unordered_map<string, double>>
fetch_all_rates_parallel(const string& api_key, const vector<string>& currencies) {
    unordered_map<string, unordered_map<string, double>> data;
    CURLM* multi_handle = curl_multi_init();
    vector<CURL*> easy_handles;
    unordered_map<CURL*, pair<string, string*>> handle_map;
    for (const string& base : currencies) {
        CURL* eh = curl_easy_init();
        string* response = new string();
        string url = "https://v6.exchangerate-api.com/v6/" + api_key + "/latest/" + base;
        curl_easy_setopt(eh, CURLOPT_URL, url.c_str());
        curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(eh, CURLOPT_WRITEDATA, response);
        curl_easy_setopt(eh, CURLOPT_TIMEOUT, 10L);
        curl_multi_add_handle(multi_handle, eh);
        easy_handles.push_back(eh);
        handle_map[eh] = {base, response};
    }
    int still_running = 0;
    curl_multi_perform(multi_handle, &still_running);
    while (still_running) {
        curl_multi_perform(multi_handle, &still_running);
    }
    for (CURL* eh : easy_handles) {
        auto [base, response] = handle_map[eh];
        try {
            auto json_data = nlohmann::json::parse(*response);
            if (json_data.contains("conversion_rates")) {
                for (const auto& quote : currencies) {
                    if (base == quote) data[base][quote] = 1.0;
                    else data[base][quote] = json_data["conversion_rates"].value(quote, NAN);
                }
            }
        } catch (const std::exception& e) {
        }
        delete response;
        curl_easy_cleanup(eh);
    }
    curl_multi_cleanup(multi_handle);
    return data;
}

class Graph {
public:
    Graph(const vector<string>& nodes, const unordered_map<string, unordered_map<string, double>>& rates)
        : nodes(nodes), rates(rates) {
        for (const auto& from : nodes) {
            for (const auto& to : nodes) {
                if (rates.at(from).at(to) > 0)
                    log_weights[from][to] = -log(rates.at(from).at(to));
            }
        }
    }
    vector<vector<string>> find_arbitrage_cycles(int min_len, int max_len) {
        vector<vector<string>> found;
        for (const auto& start : nodes) {
            unordered_map<string, double> dist;
            unordered_map<string, string> prev;
            for (const auto& cur : nodes) dist[cur] = numeric_limits<double>::infinity();
            dist[start] = 0;
            for (int i = 0; i < nodes.size() - 1; ++i) {
                for (const auto& u : nodes) {
                    for (const auto& [v, w] : log_weights[u]) {
                        if (dist[u] + w < dist[v]) {
                            dist[v] = dist[u] + w;
                            prev[v] = u;
                        }
                    }
                }
            }
            for (const auto& u : nodes) {
                for (const auto& [v, w] : log_weights[u]) {
                    if (dist[u] + w < dist[v]) {
                        vector<string> cycle = {v};
                        string cur = u;
                        while (find(cycle.begin(), cycle.end(), cur) == cycle.end()) {
                            cycle.push_back(cur);
                            cur = prev[cur];
                        }
                        cycle.push_back(v);
                        reverse(cycle.begin(), cycle.end());
                        if (cycle.size() >= min_len && cycle.size() <= max_len)
                            found.push_back(cycle);
                    }
                }
            }
        }
        return found;
    }
    double evaluate_cycle_gain(const vector<string>& path) {
        double amt = 100.0;
        for (size_t i = 1; i < path.size(); ++i)
            amt *= rates[path[i - 1]][path[i]];
        return amt - 100.0;
    }
private:
    vector<string> nodes;
    unordered_map<string, unordered_map<string, double>> rates;
    unordered_map<string, unordered_map<string, double>> log_weights;
};

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    string api_key = "";
    auto t0 = chrono::high_resolution_clock::now();
    double start_time = chrono::duration<double>(t0.time_since_epoch()).count();
    auto t1 = chrono::high_resolution_clock::now();
    auto rates = fetch_all_rates_parallel(api_key, CURRENCIES);
    auto t2 = chrono::high_resolution_clock::now();
    cout << "Time to fetch all rates: "
         << chrono::duration<double>(t2 - t1).count() << " sec\n";
    auto t3 = chrono::high_resolution_clock::now();
    Graph g(CURRENCIES, rates);
    auto cycles = g.find_arbitrage_cycles(MIN_CYCLE_LENGTH, MAX_CYCLE_LENGTH);
    auto t4 = chrono::high_resolution_clock::now();
    cout << "Time to find arbitrage cycles: "
         << chrono::duration<double>(t4 - t3).count() << " sec\n";
    bool found = false;
    for (const auto& cycle : cycles) {
        double gain = g.evaluate_cycle_gain(cycle);
        if (gain > thresholdGain) {
            found = true;
            double t_now = chrono::duration<double>(chrono::high_resolution_clock::now().time_since_epoch()).count();
            cout << "Gain: " << gain << " | Path: ";
            for (size_t i = 0; i < cycle.size(); ++i) {
                cout << cycle[i];
                if (i + 1 < cycle.size()) cout << " → ";
            }
            cout << " | ⏱️ Detected after " << t_now - start_time << " sec\n";
        }
    }
    if (!found) {
        cout << "No arbitrage opportunities found.\n";
    }
    return 0;
}
