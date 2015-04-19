#pragma once
#include "../problem/base.h"
#include "../timer.h"

#include <deque>
#include <map>
#include <vector>
#include <cstdio>

namespace algorithm
{

	template<typename F, typename S>
	class tabu
	{
		problem::base<F,S>& problem;
		typedef std::deque<S> tabu_type;
		tabu_type tabu_list;
		unsigned int tabu_len, large_count, res_count;
		bool large_neigh;
	public:
		tabu(problem::base<F,S>& prob, unsigned int len, bool large_neigh = false, unsigned int large_count = 0, unsigned int res_count = 3)
			: problem(prob), tabu_len(len), large_neigh(large_neigh), large_count(large_count), res_count(res_count) {}
		~tabu() {}

		int evolve(double runtime)
		{
			timer time;
			S global_best = problem.get_solution();
			F global_best_fit = problem.fitness(global_best);
			F current_best_fit = global_best_fit;
			int count = 0;

			//std::cout << "Start fitness: " << global_best_fit << std::endl;

			unsigned int no_improvement_count = 0;
			unsigned int neigh_size = 1;
			while (time.elapsed() < runtime)
			{
				count++;
				if ((large_neigh && neigh_size == 2 && no_improvement_count == res_count) || (!large_neigh && no_improvement_count == res_count))
				{
					// Restart diversification
					problem.restart();
					current_best_fit = problem.fitness(problem.get_solution());
					neigh_size = 1;
					no_improvement_count = 0;
				}

				// Add current to Tabu list
				tabu_list.push_front(problem.get_solution());
				if (tabu_list.size() > tabu_len) tabu_list.pop_back();

				// Get list of neighbours
				if (large_neigh && neigh_size == 1 && no_improvement_count == large_count)
				{
					std::cout << "Using size 2 neighbourhood" << std::endl;
					neigh_size = 2;
					no_improvement_count = 0;
				}
				//auto neighbours = problem.neighbours(neigh_size);
				S best_n = problem.find_neigh_thread(neigh_size, this);
				F best_n_fit = problem.fitness(best_n);

				//std::cout << "Neighbour fitness: " << best_n_fit << std::endl;

				// Is new better than current global best?
				no_improvement_count++;
				if (best_n_fit < global_best_fit) // Minimization!
				{
					//std::cout << "New global best" << std::endl;
					global_best = best_n;
					global_best_fit = best_n_fit;
					current_best_fit = best_n_fit;
					no_improvement_count = 0;
				}
				else if (best_n_fit < current_best_fit) // Keep track of non-global improvements for restart diversification
				{
					current_best_fit = best_n_fit;
					no_improvement_count = 0;
				}

				// Always set best neighbour as current
				problem.set_solution(best_n);
			}

			// Set solution
			problem.set_solution(global_best);

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
			if (s.begin() == s.end()) // Empty neighbour list...
			{
				return problem.get_solution(); // Return current...
			}
			F best_score = problem.fitness(*best);
			for (auto sol = s.begin(); sol < s.end(); sol++)
			{
				F cur = problem.fitness(*sol);
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

		void print_params(std::ostream& os = std::cout)
		{
			os << "Tabu length: " << tabu_len << std::endl;
			os << "Reset count: " << res_count << std::endl;
			os << "Large neighbourhood: " << (large_neigh ? "true" : "false") << std::endl;
			if (large_neigh) os << "Large count: " << large_count << std::endl;
		}
	};
}
