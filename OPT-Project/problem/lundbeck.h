#pragma once
#include "base.h"
#include <map>
#include <vector>
#include <random>
#include <string>

namespace problem
{
	class job;

	class lundbeck : public base<double, std::vector<std::vector<unsigned int>>>
	{
	public:
		std::uniform_int_distribution<int> machine_dist;
		std::mt19937 rand;

		std::vector<std::map<std::pair<unsigned int, unsigned int>, unsigned int>> LTM; // Long term memory for diversification

		const unsigned int n_machines;
		unsigned int n_jobs;
		std::map<unsigned int, job> jobs;

		lundbeck(std::string file, const unsigned int machines);
		~lundbeck();

		virtual void set_solution(solution_type& sol) override;

		bool lundbeck::machine_job_valid(unsigned int m, unsigned int j);
		bool is_valid(const solution_type& sol);

		virtual fitness_type fitness(const solution_type& s) override;

		void move_between(std::vector<unsigned int>& f, std::vector<unsigned int>& t, int from, int to);
		void find_neigh_thread(std::vector<solution_type>& neighbours, int size);
		std::vector<solution_type> neighbours(int size) override;

		virtual void restart() override; // Diversification
		virtual void reset() override; // Reset state for param tuning

		void add_initial_solution(const job& j);

		void print_solution();
	};

	class job
	{
	public:
		unsigned int id;
		unsigned int blisters;
		unsigned int dos;
		unsigned int formulation;
		unsigned int height;
		bool hotmelt;
		unsigned int leaflet;
		unsigned int material;
		unsigned int mdvp;
		unsigned int nummeret;
		unsigned int prenumb;
		std::string packing_type;

		double strength;
		enum machinetype
		{
			ALL,
			ONE,
			TWO,
			ONETWO
		} machine_type;

		bool operator==(const job& rhs) const
		{
			return id == rhs.id;
		}
	};
}
