#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
using namespace std;
class Graph {
public:
    Graph(const vector<string>& currencies,
          const unordered_map<string, unordered_map<string, double>>& rates);

    vector<vector<string>>
    find_arbitrage_cycles(int min_length, int max_length);

    double evaluate_cycle_gain(const vector<string>& path);

private:
    vector<string> currencies;
    unordered_map<string, unordered_map<string, double>> rates;
    unordered_map<string, unordered_map<string, double>> log_weights;
};
