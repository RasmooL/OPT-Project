#include <iostream>
#include "algorithm\tabu.h"
#include "problem\lundbeck.h"

int main()
{
	using namespace std;

	bool param_tune = false;
	double run_time = 20;
	unsigned int machines = 3;
	unsigned int tabu_length = 500;
	unsigned int neigh_size = 1;

	using prob_type = problem::lundbeck;
	auto problem = prob_type("C:\\Users\\Rasmus\\Documents\\Visual Studio 2013\\Projects\\OPT-Project\\blister_actual.csv", machines);
	auto algorithm = algorithm::tabu<prob_type::fitness_type, prob_type::solution_type>(problem, tabu_length, neigh_size);

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