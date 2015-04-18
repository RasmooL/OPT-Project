#pragma once

#include <vector>
#include <cstdio>
#include <iostream>

namespace problem
{
	template<typename F, typename S>
	class base
	{
	protected:
		S solution;
	public:
		typedef F fitness_type;
		typedef S solution_type;

		S& get_solution()
		{
			return solution;
		}

		virtual void set_solution(S& sol)
		{
			solution = sol;
		}

		virtual void restart()
		{
			return;
		}

		virtual void reset()
		{
			return;
		}

		virtual F fitness() final
		{
			return fitness(solution);
		}

		virtual F fitness(const S& s)
		{
			std::cerr << "Error: Call to problem::base::fitness()" << std::endl;
			return false;
		}

		virtual std::vector<S> neighbours(int size)
		{
			return std::vector<S>(1, solution);
		}
	};
}
