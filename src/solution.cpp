﻿#include "solution.h"
#include "generator.h"
#include "support.h"
#include "config.h"

#include <iostream>
#include <algorithm>
#include <string>

using namespace std;


solution::solution(const task& data, std::chrono::duration<int> available_time, solution::init_type init)
	: _data(data), _available_time(available_time)
{
    // TODO: DANGER, REMOVE!!!!
    _previous_city_buffer.reserve(500000);

	_start = chrono::steady_clock::now();
	_cluster_count = _data.cluster_count();
	_start_city = _data.get_start_city();

	switch(init) {
		case COMPLETE_DFS:
			complete_dfs_init();
			break;
		case SHUFFLE:
			shuffle_init();
			break;
		case GREEDY_DFS:
			greedy_search_init();
			break;
        case REVERSE_GREEDY_DFS:
            reverse_greedy_search_init();
            break;
	}

	for (cluster_id_t cluster_id = 0; cluster_id < _data.cluster_count(); ++cluster_id)
	{
		std::vector<city_cost_struct> cities_cost;

		for (size_t n = 0; n < _data.get_number_of_cities(cluster_id); ++n)
		{
			cities_cost.emplace_back(_data.get_nth_city_of_cluster(cluster_id, n), MAX_TOTAL_COST, 0, 0);
		}
		_city_cost_cache.push_back(move(cities_cost));
	}

	_sum_min_cluster_costs = 0;
	_min_cluster_costs = vector<cost_t>(_cluster_count - 1, 0);

	for (int i = 0; i < _cluster_count - 1; ++i)
	{
		int cost = _data.get_cluster_cost(_clusters[(i - 1) % (_cluster_count - 1)], _clusters[i], _clusters[i + 1], i);
		cost = (int)(log2f(cost + 1) + 0.5f);
		_min_cluster_costs[i] = cost;
		_sum_min_cluster_costs += cost;
	}

	initialize_cost();

#ifdef _PRINT
	cout << _route_cost << endl;
#endif
}

void solution::permute()
{
	if (generator::rnd_float() < 0.3) more_genius_swap();
	else if (generator::rnd_float() < 0.325) distant_swap();
	else genius_swap();

	calculate_cost();
}

void solution::revert_step()
{
	swap();
	calculate_cost();
}

void solution::submit_step()
{
	recalculate_min_costs();
}

void solution::set_clusters(std::vector<cluster_id_t>&& clusters)
{
	_clusters = move(clusters);
	initialize_cost();
}

void solution::simple_swap()
{
	_swapped_1 = generator::rnd_int() % (_cluster_count - 1);
	_swapped_2 = (_swapped_1 + 1) % (_cluster_count - 1);

	if(_swapped_1 > _swapped_2)
	{
		_swapped_1 ^= _swapped_2;
		_swapped_2 ^= _swapped_1;
		_swapped_1 ^= _swapped_2;
	}

	swap();
}

void solution::distant_swap()
{
	_swapped_1 = generator::rnd_int() % (_cluster_count - 1);
	_swapped_2 = generator::rnd_int() % (_cluster_count - 1);

	if(_swapped_1 == _swapped_2)
	{
		_swapped_2 = (_swapped_2 + 1) % (_cluster_count - 1);
	}\

	if (_swapped_1 > _swapped_2)
	{
		_swapped_1 ^= _swapped_2;
		_swapped_2 ^= _swapped_1;
		_swapped_1 ^= _swapped_2;
	}

	swap();
}

void solution::clever_swap()
{
	for (size_t i = 0; i < 50; ++i)
	{
		_swapped_1 = generator::rnd_int() % (_cluster_count - 1);
		_swapped_2 = generator::rnd_int() % (_cluster_count - 1);

		if (_swapped_1 == _swapped_2)
		{
			_swapped_2 = (_swapped_2 + 1) % (_cluster_count - 1);
		}

		const auto a_p = _clusters[(_swapped_1 - 1) % (_cluster_count - 1)];
		const auto a = _clusters[_swapped_1];
		const auto a_n = _clusters[_swapped_1 + 1];

		const auto b_p = _clusters[(_swapped_2 - 1) % (_cluster_count - 1)];
		const auto b = _clusters[_swapped_2];
		const auto b_n = _clusters[_swapped_2 + 1];

		if (!_data.get_conflict(a_p, b, _swapped_1) &&
			!_data.get_conflict(b, a_n, _swapped_1 + 1) &&
			!_data.get_conflict(b_p, a, _swapped_2) &&
			!_data.get_conflict(a, b_n, _swapped_2 + 1)
			)
			break;
	}

	if (_swapped_1 > _swapped_2)
	{
		_swapped_1 ^= _swapped_2;
		_swapped_2 ^= _swapped_1;
		_swapped_1 ^= _swapped_2;
	}

	swap();
}

void solution::genius_swap()
{
	_swapped_1 = roulette_selector();

	const auto a_p = _clusters[(_swapped_1 - 1) % (_cluster_count - 1)];
	const auto a = _clusters[_swapped_1];
	const auto a_n = _clusters[_swapped_1 + 1];

	for (size_t i = 0; i < 100; ++i)
	{
		_swapped_2 = generator::rnd_int() % (_cluster_count - 1);

		if (_swapped_1 == _swapped_2)
		{
			_swapped_2 = (_swapped_2 + 1) % (_cluster_count - 1);
		}

		const auto b_p = _clusters[(_swapped_2 - 1) % (_cluster_count - 1)];
		const auto b = _clusters[_swapped_2];
		const auto b_n = _clusters[_swapped_2 + 1];

		// a_p---a---a_n  ...  b_p---b---b_n
		/*const auto conflicts_before =
			_data.get_conflict(a_p, a, _swapped_1)
			+ _data.get_conflict(a, a_n, _swapped_1 + 1)
			+ _data.get_conflict(b_p, b, _swapped_2)
			+ _data.get_conflict(b, b_n, _swapped_2 + 1);*/

		// a_p---b---a_n  ...  b_p---a---b_n
		if (!_data.get_conflict(a_p, b, _swapped_1) &&
			!_data.get_conflict(b, a_n, _swapped_1 + 1) &&
			!_data.get_conflict(b_p, a, _swapped_2) &&
			!_data.get_conflict(a, b_n, _swapped_2 + 1)
			)
			break;
	}

	if (_swapped_1 > _swapped_2)
	{
		_swapped_1 ^= _swapped_2;
		_swapped_2 ^= _swapped_1;
		_swapped_1 ^= _swapped_2;
	}

	swap();
}

void solution::more_genius_swap()
{
	_swapped_1 = roulette_selector();
	_swapped_2 = -1;

	const auto a_p = _clusters[(_swapped_1 - 1) % (_cluster_count - 1)];
	const auto a = _clusters[_swapped_1];
	const auto a_n = _clusters[_swapped_1 + 1];

	auto best_score = MAX_TOTAL_COST;

	for (size_t i = 0; i < _cluster_count - 1; ++i)
	{
		if (i == _swapped_1) continue;
		
		const auto b_p = _clusters[(i - 1) % (_cluster_count - 1)];
		const auto b = _clusters[i];
		const auto b_n = _clusters[i + 1];

		// a_p---b---a_n  ...  b_p---a---b_n
		if (_data.get_conflict(a_p, b, _swapped_1) ||
			_data.get_conflict(b, a_n, _swapped_1 + 1) ||
			_data.get_conflict(b_p, a, i) ||
			_data.get_conflict(a, b_n, i + 1)
			)
			continue;

		auto score = _data.get_cluster_cost(a_p, b, a_n, _swapped_1) + _data.get_cluster_cost(b_p, a, b_n, i);

		if(score < best_score)
		{
			_swapped_2 = i;
			best_score = score;
		}
	}

	if (_swapped_2 == -1) return distant_swap();

	if (_swapped_1 > _swapped_2)
	{
		_swapped_1 ^= _swapped_2;
		_swapped_2 ^= _swapped_1;
		_swapped_1 ^= _swapped_2;
	}

	swap();
}

void solution::initialize_cost()
{
	for (auto&& next_city : _city_cost_cache[_clusters[0]])
	{
		next_city.cost = _data.get_cost(_start_city, next_city.city, 0);
		next_city.gain_out = next_city.gain_in = 0;
	}

	for (size_t i = 1; i < _cluster_count; ++i)
	{
		for (auto&& next_city : _city_cost_cache[_clusters[i]])
		{
			next_city.cost = MAX_TOTAL_COST;
			next_city.gain_in = next_city.gain_out = 0;

			for (auto&& prev_city : _city_cost_cache[_clusters[i - 1]])
			{
				auto cost = _data.get_cost(prev_city.city, next_city.city, i);
				next_city.cost = min(next_city.cost, prev_city.cost + cost);
			}
		}
	}

	_route_cost = MAX_TOTAL_COST;
	for (auto&& end_city : _city_cost_cache[_clusters[_cluster_count - 1]])
	{
		if (end_city.cost < _route_cost) _route_cost = end_city.cost;
	}
}

void solution::calculate_cost()
{
#ifdef _DEBUG
	if (_swapped_1 >= _swapped_2) throw exception("error");
#endif

	_route_cost += _city_cost_cache[_clusters[_swapped_1]][0].gain_in - _city_cost_cache[_clusters[_swapped_1]][0].gain_out;
	_route_cost += _city_cost_cache[_clusters[_swapped_2]][0].gain_in - _city_cost_cache[_clusters[_swapped_2]][0].gain_out;

	if (_swapped_1 == 0)
	{
		for (auto&& next : _city_cost_cache[_clusters[0]])
		{
			next.cost = _data.get_cost(_start_city, next.city, 0);
			next.gain_out = next.gain_in = 0;
		}
	}

	for (int i = max(_swapped_1, 1); i < _cluster_count - 1; ++i)
	{
		if (i != _swapped_1 && i != _swapped_2 && _city_cost_cache[_clusters[i]].size() == 1)
		{
			auto& next = _city_cost_cache[_clusters[i]][0];
			auto tmp_cost = MAX_TOTAL_COST;

			for (auto&& prev : _city_cost_cache[_clusters[i - 1]])
			{
				auto cost = _data.get_cost(prev.city, next.city, i);
				tmp_cost = min(tmp_cost, prev.cost + cost);
			}

			auto& prev = _city_cost_cache[_clusters[i - 1]][0];

			int diff = (tmp_cost - next.cost) + (next.gain_in - prev.gain_out);
			next.gain_in = prev.gain_out;
			next.gain_out += tmp_cost - next.cost;
			next.cost = tmp_cost;

			_route_cost += diff;

			if (i > _swapped_2) return;
			
			i = _swapped_2 - 1;
			continue;
		}

		for (auto&& next : _city_cost_cache[_clusters[i]])
		{
			next.cost = MAX_TOTAL_COST;

			for (auto&& prev : _city_cost_cache[_clusters[i - 1]])
			{
				auto cost = _data.get_cost(prev.city, next.city, i);
				next.cost = min(next.cost, prev.cost + cost);
			}

			next.gain_in = _city_cost_cache[_clusters[i - 1]][0].gain_out;
			next.gain_out = next.gain_in;
		}
	}

	// execute only if we have not found a single-city cluster in the previous loop
	total_cost_t last_min_total_cost = MAX_TOTAL_COST;
	total_cost_t min_total_cost = MAX_TOTAL_COST;

	for (auto&& next : _city_cost_cache[_clusters[_cluster_count - 1]])
	{
		auto tmp_cost = MAX_TOTAL_COST;

		for (auto&& prev : _city_cost_cache[_clusters[_cluster_count - 2]])
		{
			auto cost = _data.get_cost(prev.city, next.city, _cluster_count - 1);
			tmp_cost = min(tmp_cost, prev.cost + cost);
		}

		last_min_total_cost = min(last_min_total_cost, next.cost);
		min_total_cost = min(min_total_cost, tmp_cost);

		next.cost = tmp_cost;
	}

	auto& next = _city_cost_cache[_clusters[_cluster_count - 1]][0];
	auto& prev = _city_cost_cache[_clusters[_cluster_count - 2]][0];

	int diff = (min_total_cost - last_min_total_cost) + (next.gain_in - prev.gain_out);
	next.gain_in = prev.gain_out;
	_route_cost += diff;
}

void solution::shuffle_init() {

	const cluster_id_t start_cluster = _data.get_start_cluster();
	for (size_t i = 0; i < _cluster_count; ++i)
	{
		if (i == start_cluster) continue;
		_clusters.push_back((cluster_id_t) i);
	}
	_clusters.push_back(start_cluster);

	//_clusters = { 10, 136, 135, 63, 74, 111, 93, 109, 81, 47, 84, 103, 78, 55, 80, 68, 99, 118, 134, 145, 1, 128, 116, 138, 73, 77, 0, 83, 31, 126, 53, 19, 125, 144, 96, 147, 32, 82, 75, 58, 35, 42, 132, 9, 76, 117, 131, 29, 27, 149, 112, 104, 140, 36, 59, 4, 97, 60, 113, 79, 56, 41, 69, 70, 61, 48, 52, 146, 89, 17, 43, 50, 123, 107, 40, 120, 105, 90, 11, 45, 3, 18, 14, 51, 46, 12, 13, 139, 33, 137, 26, 6, 30, 34, 66, 37, 98, 16, 54, 64, 119, 143, 7, 100, 72, 2, 65, 38, 114, 95, 115, 23, 122, 133, 110, 39, 24, 106, 57, 15, 8, 71, 62, 44, 129, 21, 148, 67, 25, 86, 22, 28, 49, 87, 92, 101, 130, 142, 94, 124, 20, 102, 88, 141, 108, 121, 91, 85, 5, 127 };
	shuffle(_clusters.begin(), _clusters.end() - 1, generator::random_engine);

}

void solution::reverse_greedy_search_init() {

	auto start = chrono::steady_clock::now();

	_length_multiplier_cache.reserve(_cluster_count + 1);
	for(size_t i = 0; i <= _cluster_count; ++i)
	{
		_length_multiplier_cache.push_back(1.0f / pow(i, config::GREEDY_SEARCH_EXP));
	}

	auto cmp = [&](const path_struct& left, const path_struct& right) {

		auto cost1 = left.cost * _length_multiplier_cache[left.length];
		auto cost2 = right.cost * _length_multiplier_cache[right.length];

		if (cost1 == cost2) {
			return right.length > left.length;
		}

		return cost1 > cost2;
	};

	const cluster_id_t start_cluster = _data.get_start_cluster();

	size_t solutions = 0;
	size_t i = 0;

	bool no_solution = true;
	path_struct best_solution;
	queue<path_struct, decltype(cmp)> q(cmp);
	// TODO: DANGER, REMOVE!!!!
	_previous_city_buffer.emplace_back(0, 0); // dummy value for end indication
	for (auto&& end : _data.get_cluster_cities(start_cluster)) {
		q.push(path_struct(end, start_cluster, _cluster_count, _previous_city_buffer));
	}

	while (!q.empty() && chrono::steady_clock::now() - start < _available_time)
	{
		++i;

#ifdef _PRINT
		if (i % 100000 == 0) cout << "Population: " << q.size() << ", Best: " << (no_solution ? 0 : best_solution.cost) << ", Solutions: " << solutions << endl;
#endif

		path_struct path = q.pop();

		if (path.cost >= best_solution.cost) continue;

		// const auto& edges = _data.get_edges(path.head->city, (cost_t)path.length);
		// TODO: DANGER, REMOVE!!!!
		const auto& edges = _data.get_reverse_edges(_previous_city_buffer[path.head].city, (int)_cluster_count - (int)path.length - 1);

		if (path.length == _cluster_count - 1) {

			for (auto&& e : edges) {

				if (e.first != _start_city) continue;
				path.add(_start_city, start_cluster, e.second, _previous_city_buffer);

				if (no_solution) {
					best_solution = std::move(path);
					no_solution = false;
				}
				else if (path.cost < best_solution.cost) {
					best_solution = std::move(path);
				}

				++solutions;
				break;
			}
			continue;
		}

		int K = min((int)edges.size(), max(config::GREEDY_SEARCH_KNBRS, 12 - int(path.length)));
		for (int i = 0, j = 0; j < K && i < edges.size(); ++i) {

			auto& edge = edges[i];

			if (path.visited_clusters[_data.get_city_cluster(edge.first)] || path.cost + edge.second >= best_solution.cost) continue;

			path_struct path_copy = path;
			path_copy.add(edge.first, _data.get_city_cluster(edge.first), edge.second, _previous_city_buffer);
			q.push(path_copy);
			++j;
		}
	}

	solutions_tried = i;

	if (no_solution) {
		shuffle_init();
		return;
	}

	// TODO: DANGER, REMOVE!!!!
	city_struct city = _previous_city_buffer[best_solution.head];
	while (city.prev != 0)
	{
		_clusters.push_back(_data.get_city_cluster(city.city));
		city = _previous_city_buffer[city.prev];
	}

}

void solution::greedy_search_init() {

    auto start = chrono::steady_clock::now();

	_length_multiplier_cache.reserve(_cluster_count + 1);
	for(size_t i = 0; i <= _cluster_count; ++i)
	{
		_length_multiplier_cache.push_back(1.0f / pow(i, config::GREEDY_SEARCH_EXP));
	}

	auto cmp = [&](const path_struct& left, const path_struct& right) {

		auto cost1 = left.cost * _length_multiplier_cache[left.length];
		auto cost2 = right.cost * _length_multiplier_cache[right.length];

		if (cost1 == cost2) {
			return right.length > left.length;
		}

		return cost1 > cost2;
	};

    const cluster_id_t start_cluster = _data.get_start_cluster();

    size_t solutions = 0;
	size_t i = 0;

	bool no_solution = true;
	path_struct best_solution;
	queue<path_struct, decltype(cmp)> q(cmp);
	// TODO: DANGER, REMOVE!!!!
	q.push(path_struct(_start_city, start_cluster, _cluster_count, _previous_city_buffer));

	while (!q.empty() && chrono::steady_clock::now() - start < _available_time)
	{
		++i;

#ifdef _PRINT
		if (i % 100000 == 0) cout << " Population: " << q.size() << ", Best: " << (no_solution ? 0 : best_solution.cost) << ", Solutions: " << solutions << endl;
#endif
			
		path_struct path = q.pop();

		if (path.cost >= best_solution.cost) continue;

		// const auto& edges = _data.get_edges(path.head->city, (cost_t)path.length);
		// TODO: DANGER, REMOVE!!!!
		const auto& edges = _data.get_edges(_previous_city_buffer[path.head].city, (cost_t)path.length);

		if (path.length == _cluster_count - 1) {

			for (auto&& e : edges) {

				if (_data.get_city_cluster(e.first) != start_cluster) continue;
				path.add(e.first, _data.get_city_cluster(e.first), e.second, _previous_city_buffer);

				if (no_solution) {
					best_solution = std::move(path);
					no_solution = false;
				}
				else if (path.cost < best_solution.cost) {
					best_solution = std::move(path);
				}

				++solutions;
				break;
			}
			continue;
		}

#ifdef K_NEIGHBOURS

		int K = min((int)edges.size(), max(config::GREEDY_SEARCH_KNBRS, 12 - int(path.length) - 100));

		for (int i = 0, j = 0; j < K && i < edges.size(); ++i) {

			auto& edge = edges[i];

			if (path.visited_clusters[_data.get_city_cluster(edge.first)] || path.cost + edge.second >= best_solution.cost) continue;

			path_struct path_copy = path;
			path_copy.add(edge.first, _data.get_city_cluster(edge.first), edge.second, _previous_city_buffer);
			q.push(path_copy);
			++j;
		}
#endif
#ifndef K_NEIGHBOURS
		float average = 0.0;
		cost_t min = INVALID_ROUTE;
		for (auto e : edges) {
			average += e.second;
			if (e.second < min) min = e.second;
		}
		average /= edges.size();
		auto threshold = min + (average - min) * config::GREEDY_SEARCH_RATIO;
		int used = 0;

		for (auto&& e : edges) {

			if (e.second > threshold && used >= std::min(edges.size(), (size_t)config::GREEDY_SEARCH_KNBRS)) break;

			if (path.visited_clusters[_data.get_city_cluster(e.first)] ||
			 	path.cost + e.second >= best_solution.cost) continue;

			path_struct path_copy = path;
			path_copy.add(e.first, _data.get_city_cluster(e.first), e.second, _previous_city_buffer);
			q.push(path_copy);
			++used;
		}

#endif
	}

	solutions_tried = i;

	if (no_solution) {
		shuffle_init();
		return;
	}

	// MEMORY LEAK! but intented, 'cause who cares in the limited time
//	city_struct* city = best_solution.head;
//
//	while(city->prev != nullptr)
//	{
//		_clusters.push_back(_data.get_city_cluster(city->city));
//		city = city->prev;
//	}

	config::INITIAL_TEMP /= 2;

	// TODO: DANGER, REMOVE!!!!
	city_struct city = _previous_city_buffer[best_solution.head];
	while (city.prev != 0 || city.city != _start_city)
	{
		_clusters.push_back(_data.get_city_cluster(city.city));
		city = _previous_city_buffer[city.prev];
	}

	reverse(_clusters.begin(), _clusters.end());
}

size_t solution::roulette_selector()
{
	const int rnd = (generator::rnd_int() % _sum_min_cluster_costs) + 1;
	int sum = 0;

	for(size_t i = 0; i < _cluster_count - 1; ++i)
	{
		sum += _min_cluster_costs[i];
		if (sum > rnd) return i;
	}
	return _cluster_count - 2;
}

void solution::recalculate_min_costs()
{
	if(_swapped_1 > 0) update_min_cost(_swapped_1 - 1);

	update_min_cost(_swapped_1);

	if (_swapped_1 + 1 != _swapped_2) update_min_cost(_swapped_1 + 1);
	else if(_swapped_1 + 2 != _swapped_2) update_min_cost(_swapped_2 - 1);

	update_min_cost(_swapped_2);
	
	if (_swapped_2 < _cluster_count - 2) update_min_cost(_swapped_2 + 1);
}

void solution::complete_dfs_init() {

	vector<int> visited_clusters(_cluster_count, -1);
	total_cost_t best_cost = MAX_TOTAL_COST;
	vector<int> best_path;

	visited_clusters[_data.get_city_cluster(_start_city)] = (int)_cluster_count - 1;
	for (auto&& e : _data.get_edges(_start_city, 0)) {
		if (visited_clusters[_data.get_city_cluster(e.first)] != -1) continue;
		complete_dfs_init_recursive(e.first, e.second, 0, visited_clusters, best_cost, best_path);
	}

	for (auto i : sort_indexes(best_path)) {
		_clusters.push_back((cluster_id_t) i);
	}
}

void solution::complete_dfs_init_recursive(city_id_t city, total_cost_t cost, int length, vector<int>& visited_clusters, total_cost_t& best_cost, vector<int>& best_path) {

	if (cost > best_cost) return;

	visited_clusters[_data.get_city_cluster(city)] = length++;
	auto& edges = _data.get_edges(city, length);

	if (length == _cluster_count - 1) {

		cost_t min = INVALID_ROUTE;
		for (auto&& e : edges) {
			if (_data.get_city_cluster(e.first) != _data.get_city_cluster(_start_city) || e.second > min) continue;
			min = e.second;
		}

		if (min != INVALID_ROUTE && best_cost > cost + min) {
			best_cost = cost + min;
			best_path = visited_clusters;
		}

	} else {

		for (auto&& e : edges) {
			if (visited_clusters[_data.get_city_cluster(e.first)] != -1) continue;
			complete_dfs_init_recursive(e.first, cost + e.second, length, visited_clusters, best_cost, best_path);
		}
	}

	visited_clusters[_data.get_city_cluster(city)] = -1;
}

std::vector<city_id_t> solution::path() {

	vector<city_id_t> path;
	path.reserve(_cluster_count);

	total_cost_t min_cost = MAX_TOTAL_COST;
	city_id_t min_city = 0;
	for (auto&& end : _city_cost_cache[_clusters[_cluster_count - 1]]) {
		if (end.cost >= min_cost) continue;
		min_city = end.city;
		min_cost = end.cost;
	}

	path.push_back(min_city);
	city_id_t next_city;
	for (int i = (int)_cluster_count - 2; i >= 0 ; --i) {

		next_city = min_city;
		min_cost = MAX_TOTAL_COST;

		for (auto&& prev : _city_cost_cache[_clusters[i]]) {
			for (auto&& next : _city_cost_cache[_clusters[i + 1]]) {
				if (next.city != next_city || prev.cost >= min_cost ||
				    _data.get_cost(prev.city, next_city, i + 1) == INVALID_ROUTE) continue;
				min_cost = prev.cost;
				min_city = prev.city;
			}
		}

		path.push_back(min_city);
	}

	path.push_back(_start_city);

	reverse(path.begin(), path.end());
	return path;
}

void solution::print(std::ostream &output) {

	output << cost() << endl;
	std::vector<city_id_t> p = path();

	for (size_t i = 0; i < _cluster_count; i++)
	{
		output << _data.get_city_name(p[i]) << " ";
		output << _data.get_city_name(p[i + 1]) << " ";
		output << i + 1 << " ";
		output << _data.get_cost(p[i], p[i + 1], i) << endl;
	}
}
