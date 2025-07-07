// fetcher.hpp
#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
using namespace std;

unordered_map<string, unordered_map<string, double>>
fetch_all_rates(const string& api_key, const vector<string>& currencies);