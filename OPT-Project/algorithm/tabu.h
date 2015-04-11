#pragma once
#include "../problem/base.h"
#include "../timer.h"

#include <list>
#include <deque>
#include <cstdio>

namespace algorithm
{
	template<typename F, typename S>
	class tabu
	{
		problem::base<F,S>& problem;
		typedef std::deque<S> tabu_type;
		tabu_type tabu_list;
		unsigned int tabu_len, neigh_size;
	public:
		tabu(problem::base<F,S>& prob, unsigned int len, unsigned int neigh_size) : problem(prob), tabu_len(len), neigh_size(neigh_size) {}
		~tabu() {}

		int evolve(double runtime)
		{
			timer time;
			S& global_best = problem.solution;
			int count = 0;

			std::cout << "Start fitness: " << problem.fitness(global_best) << std::endl;

			while (time.elapsed() < runtime)
			{
				count++;

				// Add current to Tabu list
				tabu_list.push_front(problem.solution);
				if (tabu_list.size() > tabu_len) tabu_list.pop_back();

				// Get list of neighbours
				if (count > 500) neigh_size = 2;
				auto neighbours = problem.neighbours(neigh_size);

				// Choose best one
				S& best_n = get_best(neighbours, problem.fitness(global_best));

				std::cout << "Neighbour fitness: " << problem.fitness(best_n) << std::endl;

				// Is new better than current global best?
				if (problem.compare_fitness(best_n))
				{
					std::cout << "New global best" << std::endl;
					global_best = best_n;
				}

				// Always set best neighbour as current
				problem.solution = best_n;
			}

			// Set solution
			problem.solution = global_best;

			return count;
		}

		// Finds the best solution in s, excluding the Tabu list
		S& get_best(std::vector<S>& s, F global_best)
		{
			auto best = s.begin();
			F best_score = problem.fitness(*best);
			for (auto sol = s.begin(); sol < s.end(); sol++)
			{
				F cur = problem.fitness(*sol);
				if (cur < best_score)// && std::find<S>(tabu_list.begin(), tabu_list.end(), *sol) == tabu_list.end()) // Better than current best & not in Tabu list
				{
					best = sol;
					best_score = problem.fitness(*best);
				}
				else if (cur < best_score && cur < global_best)
				{
					std::cout << "Disregarded tabu list (global best)." << std::endl;

					best = sol;
					best_score = problem.fitness(*best);
				}
			}
			return *best;
		}
	};
}
