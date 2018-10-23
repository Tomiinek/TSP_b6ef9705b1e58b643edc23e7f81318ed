#include <iostream>
#include <stdio.h>
#include <cmath>

#include "test.h"
#include "task.h"
#include "annealing.h"


using namespace std;

void test::check_performance()
{
	//check_performance("resources\\TSALESMAN2-1.in");
	//check_performance("resources\\TSALESMAN2-2.in");
	check_performance("resources\\TSALESMAN2-3.in");
	check_performance("resources\\TSALESMAN2-4.in");
}

void test::check_performance(const char *path)
{
	cout << path << endl << endl;

    FILE *file;
    file = fopen(path, "r");
    if (file == nullptr) perror("Error opening file");

	task t;
	t.load(file);

	const auto max_duration = t.get_available_time();

	double avg_score = 0.0;
	double var_score = 0.0;
	double min_score = std::numeric_limits<double>::max();
	double max_score = 0.0;

	double avg_speed = 0.0;
	double var_speed = 0.0;
	double min_speed = std::numeric_limits<double>::max();
	double max_speed = 0.0;

	const size_t N = 10;

	for (size_t i = 1; i <= N; ++i)
	{
		cout << "\r" << i;

		annealing s(t, max_duration, "stats.out");
		solution solution(t);
		s.run(solution);


		auto score = solution.cost();

		auto new_avg_score = avg_score + (score - avg_score) / i;
		var_score += (score - avg_score)*(score - new_avg_score);
		avg_score = new_avg_score;

		if (score < min_score) min_score = score;
		if (score > max_score) max_score = score;

		auto time = s.time.count() / 1000000000.0f;
		auto speed = s.permutations / time;

		auto new_avg_speed = avg_speed + (speed - avg_speed) / i;
		var_speed += (speed - avg_speed)*(speed - new_avg_speed);
		avg_speed = new_avg_speed;

		if (speed < min_speed) min_speed = speed;
		if (speed > max_speed) max_speed = speed;

		cout << "\t" << avg_score;
	}  

	cout << "\r";
	cout << "score average: " << avg_score << endl;
	cout << "score std deviation: " << sqrt(var_score / double(N)) << endl;
	cout << "score min: " << min_score << endl;
	cout << "score max: " << max_score << endl;

	cout << "speed average: " << avg_speed << endl;
	cout << "speed std deviation: " << sqrt(var_speed / double(N)) << endl;
	cout << "speed min: " << min_speed << endl;
	cout << "speed max: " << max_speed << endl;

	cout << endl << "===========================================" << endl << endl << endl;
}
