#include <iostream>
#include "algorithm\tabu.h"
#include "problem\lundbeck.h"

int main()
{
	using namespace std;

	bool param_tune = false;
	double run_time = 180;
	unsigned int machines = 3;
	unsigned int tabu_length = 1000;
	bool large_neigh = true;
	unsigned int large_count = 1000;

	using prob_type = problem::lundbeck;
	using alg_type = algorithm::tabu<prob_type::fitness_type, prob_type::solution_type>;
	auto problem = prob_type("C:\\Users\\Rasmus\\Documents\\Visual Studio 2013\\Projects\\OPT-Project\\blister_actual.csv", machines);
	auto algorithm = alg_type(problem, tabu_length, large_neigh, large_count);

	// Parameter tune, or calculate solution
	if (param_tune)
	{
		// TODO
	}
	else
	{
		int iter = algorithm.evolve(run_time);

		problem.print_solution();

		cout << "Iterations: " << iter << endl;
		cout << "Is valid solution: " << problem.is_valid(problem.solution) << std::endl;
	}

	cin.get();
	return 0;
}