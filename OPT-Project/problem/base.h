#pragma once

#include <vector>
#include <cstdio>
#include <iostream>
//#include "../algorithm/tabu.h"

namespace algorithm
{
	template<typename F, typename S> class tabu;
}

namespace problem
{
	template<typename F, typename S>
	class base
	{
	public:
		typedef F fitness_type;
		typedef S solution_type;

		S solution;

		virtual F fitness() final
		{
			return fitness(solution);
		}

		virtual F fitness(const S& s)
		{
			std::cerr << "Error: Call to problem::base::fitness()" << std::endl;
			return false;
		}

		bool compare_fitness(const S& s)
		{
			return fitness(s) < fitness(); // Minimization!
		}

		//virtual std::vector<S> neighbours(int size)
		//{
		//	return std::vector<S>(1, solution);
		//}

		virtual S find_neigh_thread(int size, algorithm::tabu<F,S>* const tabu)
		{
			return S();
		}

		void set_bounds(double min, double max);
	};
}
