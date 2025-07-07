#include "graph.hpp"
#include <cmath>
#include <limits>
#include <iostream>
#include <unordered_set>
using namespace std;
Graph::Graph(const vector<string>& currencies,
             const unordered_map<string, unordered_map<string, double>>& rates)
    : currencies(currencies), rates(rates) {
    for (const auto& from : currencies) {
        for (const auto& to : currencies) {
            if (rates.at(from).at(to) > 0)
                log_weights[from][to] = -log(rates.at(from).at(to));
        }
    }
}

vector<vector<string>>
Graph::find_arbitrage_cycles(int min_length, int max_length) {
    cout<<"graph was called\n";
    vector<vector<string>> cycles;
    for (const auto& start : currencies) {
        unordered_map<string, double> dist;
        unordered_map<string, string> prev;
        for (const auto& cur : currencies) dist[cur] = numeric_limits<double>::infinity();
        dist[start] = 0;

        for (int i = 0; i < currencies.size() - 1; ++i) {
            for (const auto& u : currencies) {
                for (const auto& [v, w] : log_weights[u]) {
                    if (dist[u] + w < dist[v]) {
                        dist[v] = dist[u] + w;
                        prev[v] = u;
                    }
                }
            }
        }

        for (const auto& u : currencies) {
            for (const auto& [v, w] : log_weights[u]) {
                if (dist[u] + w < dist[v]) {
                    vector<string> cycle = {v};
                    string curr = u;
                    while (find(cycle.begin(), cycle.end(), curr) == cycle.end()) {
                        cycle.push_back(curr);
                        curr = prev[curr];
                    }
                    cycle.push_back(v);
                    reverse(cycle.begin(), cycle.end());
                    if (cycle.size() >= min_length && cycle.size() <= max_length)
                        cycles.push_back(cycle);
                }
            }
        }
    }
    return cycles;
}

double Graph::evaluate_cycle_gain(const vector<string>& path) {
    double amt = 100.0;
    for (size_t i = 1; i < path.size(); ++i)
        amt *= rates[path[i - 1]][path[i]];
    return amt - 100.0;
}