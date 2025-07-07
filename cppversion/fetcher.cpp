#include "fetcher.hpp"
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>
#include <curl/curl.h>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using namespace std;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

unordered_map<string, unordered_map<string, double>>
fetch_all_rates(const string& api_key, const vector<string>& currencies) {
    cout<<"fetch_all_rates was called\n";
    unordered_map<string, unordered_map<string, double>> data;

    CURL* curl = curl_easy_init();
    if (!curl) {
        cerr << "Failed to initialize CURL" << endl;
        return data;
    }

    for (const auto& base : currencies) {
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

        try {
            auto json_data = json::parse(response_string);
            if (json_data.contains("conversion_rates")) {
                for (const auto& quote : currencies) {
                    if (base == quote) data[base][quote] = 1.0;
                    else data[base][quote] = json_data["conversion_rates"].value(quote, NAN);
                }
            }
        } catch (const std::exception& e) {
            cerr << "JSON parse error for " << base << ": " << e.what() << endl;
        }
    }

    curl_easy_cleanup(curl);
    return data;
}