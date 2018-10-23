#ifndef TSP_CHALLENGE_KIWI_SEARCHER_H
#define TSP_CHALLENGE_KIWI_SEARCHER_H

#include "task.h"
#include "solution.h"

#include <chrono>
#include <fstream>


using energy_t = int;
using temp_t = double;

class annealing
{
public:
	annealing(const task& data, std::chrono::duration<int> available_time, const std::string& stats_path);

	void run(solution& s);

	size_t permutations;
	duration_t time;

private:

	float acceptance_probability(energy_t current, energy_t next) const;
	void update_temperature();

	/* resources\TSALESMAN2-3.in, INITIAL_TEMP = 1*150, COOLING_TEMP = 0.9999998
	 *
	 * score average: 40115.9
	 * score std deviation: 704.642
	 * score min: 39065
	 * score max: 41388
	 *
	 */

	// ANNEALING PARAMS:
	const temp_t INITIAL_TEMP = 0.8; //best: 0.3
	const double COOLING_TEMP = 0.9999993; // best: 0.9999995
	temp_t _t;

	// general:
	const task& _data;
	const std::chrono::duration<int> _available_time;
    std::ofstream _stats;
	std::chrono::time_point<std::chrono::steady_clock> _start;
};


#endif