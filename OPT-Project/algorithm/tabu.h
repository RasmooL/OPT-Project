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
		unsigned int tabu_len, large_count;
		bool large_neigh;
	public:
		tabu(problem::base<F,S>& prob, unsigned int len, bool large_neigh = false, unsigned int large_count = 0) : problem(prob), tabu_len(len), large_neigh(large_neigh), large_count(large_count) {}
		~tabu() {}

		int evolve(double runtime)
		{
			timer time;
			S& global_best = problem.solution;
			int count = 0;

			std::cout << "Start fitness: " << problem.fitness(global_best) << std::endl;

			unsigned int no_improvement_count = 0;
			unsigned int neigh_size = 1;
			while (time.elapsed() < runtime)
			{
				count++;

				// Add current to Tabu list
				tabu_list.push_front(problem.solution);
				if (tabu_list.size() > tabu_len) tabu_list.pop_back();

				// Get list of neighbours
				if (large_neigh && neigh_size == 1 && no_improvement_count >= large_count) neigh_size = 2;
				//auto neighbours = problem.neighbours(neigh_size);
				S best_n = problem.find_neigh_thread(neigh_size, this);

				// Choose best one
				//S& best_n = get_best(neighbours, problem.fitness(global_best));

				std::cout << "Neighbour fitness: " << problem.fitness(best_n) << std::endl;

				// Is new better than current global best?
				no_improvement_count++;
				if (problem.compare_fitness(best_n))
				{
					std::cout << "New global best" << std::endl;
					global_best = best_n;
					no_improvement_count = 0;
				}

				// Always set best neighbour as current
				problem.solution = best_n;
			}

			// Set solution
			problem.solution = global_best;

			return count;
		}
		
		// Custom find function is necessary to compile with g++ (std::find works in VC++)
		typename tabu_type::iterator find_tabu(typename tabu_type::iterator first, typename tabu_type::iterator last, const S& val)
		{
		  while (first!=last) {
		    if (*first==val) return first;
		    ++first;
		  }
		  return last;
		}
		bool find_tabu(S& s)
		{
			return find_tabu(tabu_list.begin(), tabu_list.end(), s) != tabu_list.end();
		}
		// Finds the best solution in s, excluding the Tabu list
		S& get_best(std::vector<S>& s, F global_best)
		{
			auto best = s.begin();
			F best_score = problem.fitness(*best);
			for (auto sol = s.begin(); sol < s.end(); sol++)
			{
				F cur = problem.fitness(*sol);
				// TODO: FIX FIND ON GCC
				if (cur < best_score && find_tabu(tabu_list.begin(), tabu_list.end(), *sol) == tabu_list.end()) // Better than current best & not in Tabu list
				{
					best = sol;
					best_score = problem.fitness(*best);
				}
				// Global aspiration is useless with exact tabu list?
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
