from math import log
import pandas as pd
import requests
from multiprocessing import Pool, cpu_count
from concurrent.futures import ThreadPoolExecutor, as_completed
import time
import random

API_KEY = ''  

CURRENCIES = [
    'USD', 'EUR', 'JPY', 'GBP', 'CNY', 'AUD', 'CAD', 'CHF', 'HKD', 'SGD',
    'INR', 'RUB', 'BRL', 'ZAR', 'NZD', 'SEK', 'NOK', 'DKK', 'PLN', 'THB',
    'IDR', 'MYR', 'PHP', 'KRW', 'ILS', 'CZK', 'HUF', 'MXN', 'TRY', 'SAR'
]

CHUNKS = 8
CHUNK_SIZE = 16

MIN_CYCLE_LENGTH = 3
MAX_CYCLE_LENGTH = 5

def fetch_rates_for_currency(api_key, base, currencies):
    url = f"https://v6.exchangerate-api.com/v6/{api_key}/latest/{base}"
    response = requests.get(url, timeout=5)
    if response.status_code != 200:
        raise Exception(f"API request failed for {base} with status code {response.status_code}")
    rates = response.json()['conversion_rates']
    return base, {target: rates.get(target, float('nan')) for target in currencies}

def get_currency_rates(api_key, currencies):
    data = pd.DataFrame(index=currencies, columns=currencies)
    with ThreadPoolExecutor(max_workers=10) as executor:
        futures = [executor.submit(fetch_rates_for_currency, api_key, base, currencies) for base in currencies]
        for future in as_completed(futures):
            base, rates_dict = future.result()
            for target in currencies:
                data.at[base, target] = rates_dict[target] if base != target else 1.0
    return data.astype(float)

def build_neg_log_rates(rates):
    return -rates.applymap(log)

def bellman_ford(currencies, rates_matrix, log_margin=0.001, start_time=None):
    n = len(currencies)
    min_dist = [float('inf')] * n
    pre = [-1] * n
    min_dist[0] = 0

    for _ in range(n - 1):
        for u in range(n):
            for v in range(n):
                if min_dist[v] > min_dist[u] + rates_matrix.iat[u, v]:
                    min_dist[v] = min_dist[u] + rates_matrix.iat[u, v]
                    pre[v] = u

    cycles = []
    for u in range(n):
        for v in range(n):
            if min_dist[v] > min_dist[u] + rates_matrix.iat[u, v] + log_margin:
                cycle = [v]
                cur = u
                while cur not in cycle:
                    cycle.append(cur)
                    cur = pre[cur]
                cycle.append(v)
                cycle.reverse()
                named_cycle = [currencies[i] for i in cycle]
                if MIN_CYCLE_LENGTH <= len(named_cycle) <= MAX_CYCLE_LENGTH and named_cycle not in [c[0] for c in cycles]:
                    detection_time = time.time() - start_time if start_time else None
                    cycles.append((named_cycle, detection_time))
    return cycles

def evaluate_cycle_gain(path, rates):
    amt = 100
    start = path[0]
    for i in range(1, len(path)):
        amt *= rates.at[start, path[i]]
        start = path[i]
    return amt - 100

def process_chunk(args):
    chunk, chunk_rates, chunk_neg_log, start_time = args
    try:
        cycles = bellman_ford(chunk, pd.DataFrame(chunk_neg_log, index=chunk, columns=chunk), start_time=start_time)
        profitable = []
        df_rates = pd.DataFrame(chunk_rates, index=chunk, columns=chunk)
        for cycle, detection_time in cycles:
            gain = evaluate_cycle_gain(cycle, df_rates)
            if gain > 0.23:
                print(f"Gain: {gain:.2f} | Path: {' → '.join(cycle)} | ⏱️ Detected after {detection_time:.2f} sec")
                profitable.append((gain, cycle, detection_time))
        return profitable
    except Exception as e:
        print(f"Error in chunk {chunk}: {e}")
        return []

if __name__ == "__main__":
    start_time = time.time()

    try:
        fetch_start = time.time()
        rates = get_currency_rates(API_KEY, CURRENCIES)
        neg_log_rates = build_neg_log_rates(rates)
        print(f" Rates fetched and processed in {time.time() - fetch_start:.2f} sec")
    except Exception as e:
        print(f" Failed to fetch rates: {e}")
        exit(1)

    chunk_prep_start = time.time()
    chunks = [random.sample(CURRENCIES, CHUNK_SIZE) for _ in range(CHUNKS)]
    chunk_args = [
        (
            chunk,
            rates.loc[chunk, chunk].values,
            neg_log_rates.loc[chunk, chunk].values,
            start_time
        )
        for chunk in chunks
    ]
    print(f"⏱️ Chunks prepared in {time.time() - chunk_prep_start:.2f} sec")

    parallel_start = time.time()
    with Pool(processes=CHUNKS) as pool:
        results = pool.map(process_chunk, chunk_args)
    print(f"⏱️ Parallel processing in {time.time() - parallel_start:.2f} sec")

    print_start = time.time()
    all_opportunities = [item for sublist in results for item in sublist]
    if all_opportunities:
        print("\n💰 Profitable Arbitrage Cycles:")
       
    else:
        print("No arbitrage opportunities found.")
    print(f" Output printed in {time.time() - print_start:.2f} sec")

    print(f"\n Total time: {time.time() - start_time:.2f} seconds")
