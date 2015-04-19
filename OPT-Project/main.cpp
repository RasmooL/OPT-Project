#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include "boost/math/distributions/chi_squared.hpp"
#include "boost/math/distributions/students_t.hpp"
#include "algorithm/tabu.h"
#include "problem/lundbeck.h"
#include "ranker.h"

template<class T, size_t N> size_t size(T(&)[N]) { return N; }
int main()
{
	using namespace std;

	bool param_tune = true;
	unsigned int machines = 3;

#ifdef _WIN32
	std::string path = "C:\\Users\\Rasmus\\Documents\\Visual Studio 2013\\Projects\\OPT-Project\\blister_actual.csv";
#else
	std::string path = "blister_actual.csv";
#endif

	using prob_type = problem::lundbeck;
	using alg_type = algorithm::tabu<prob_type::fitness_type, prob_type::solution_type>;
	auto problem = prob_type(path, machines);

	// Parameter tune, or calculate solution
	if (param_tune)
	{
		fstream results("results.txt", ios::out | ios::trunc);

		unsigned int tabu_lengths[] = { 5, 25, 100, 250 }; // Integer
		unsigned int res_counts_small[] = { 5, 25, 100, 250 }; // Integer
		unsigned int res_counts_large[] = { 2, 5, 8 }; // Integer
		bool large_neighs[] = { false }; // Ordinal
		unsigned int large_counts[] = { 250 }; // Conditional - only if large_neighs = true

		double run_time = 600; // Fixed
		double alpha = 0.1; // Confidence

		// Build candidate algorithm list and give each an ID
		map<int, alg_type> candidates;
		int id = 0;
		for (int tl = 0; tl < size(tabu_lengths); tl++)
		{
			unsigned int ptl = tabu_lengths[tl];
			for (int rc = 0; rc < size(res_counts_small); rc++)
			{
				for (int ln = 0; ln < size(large_neighs); ln++)
				{
					bool pln = large_neighs[ln];
					unsigned int prc = pln ? res_counts_large[rc] : res_counts_small[rc];
					if (pln == true)
					{
						for (int lc = 0; lc < size(large_counts); lc++)
						{
							unsigned int plc = large_counts[lc];
							candidates.insert(make_pair(id++, alg_type(problem, ptl, pln, plc, prc)));
						}
					}
					else
					{
						candidates.insert(make_pair(id++, alg_type(problem, ptl, pln, 0, prc)));
					}
				}
			}
		}

		// Log configurations
		results << "Candidate configurations: " << endl;
		for (auto i = candidates.begin(); i != candidates.end(); i++)
		{
			int id = get<0>(*i);
			auto alg = get<1>(*i);

			results << id << endl;
			alg.print_params(results);
			results << endl;
		}

		int k = 0; // Current block number
		int k_max = 8;
		int m_max = (int)candidates.size();
		vector<map<int, double>> costs;
		vector<map<int, double>> ranks;
		vector<double> T;

		cout << "Running with " << m_max << " candidates." << endl;
		while (candidates.size() > 1 && k < k_max) // Stop if best candidate found, or exceeded maximum block number
		{
			int m = (int)candidates.size(); // Number of candidates remaining
			int kf = k + 1; // This is the 1-indexed k to be used in most places of the equations (but not for vector indexing)
			prob_type::solution_type block_sol = problem.get_solution();
			costs.push_back(map<int, double>());
			ranks.push_back(map<int, double>());

			vector<int> ids;
			for (auto i = candidates.begin(); i != candidates.end(); i++) // Evaluate each remaining candidate
			{
				int id = get<0>(*i);
				ids.push_back(id);

				auto alg = get<1>(*i);
				int iter = alg.evolve(run_time);

				//alg.print_params();
				//problem.print_solution();
				costs[k][id] = problem.fitness(problem.get_solution());
				cout << "Final fitness: " << costs[k][id] << ". ";

				cout << "Going to next candidate" << endl;
				//cin.get();
				problem.set_solution(block_sol); // Set solution back to the same for whole block
			}

			// First, do a Friedman test (family-wise)
			get_ranks(costs[k], ranks[k]);
			cout << "Ranks: ";
			for (auto r : ranks[k]) cout << r.second << " ";
			cout << endl;

			map<int, double> Rj; // Sum of ranks for each configuration
			for (int i: ids) // For each configuration
			{
				Rj[i] = 0;
				for (int l = 0; l <= k; l++) // Sum over blocks
				{
					Rj[i] += ranks[l][i];
				}
				cout << Rj[i] << endl;
			}

			double Tnum = 0; // Numerator of T
			for (int j: ids)
			{
				Tnum += pow(Rj[j] - (kf*(m+1))/2, 2.0);
			}
			Tnum *= (m - 1);
			
			double Tden = 0; // Denominator of T
			for (int l = 0; l <= k; l++)
			{
				for (int j: ids)
				{
					Tden += pow(ranks[l][j], 2.0);
				}
			}
			Tden -= (kf*m*pow(m + 1, 2.0)) / 4;
			if (Tden == 0) // Special case: if all combinations are equal, assume chi = 0 (p = 1), this has no impact on results
			{
				Tnum = 0;
				Tden = 1; 
			}

			T.push_back(Tnum / Tden);
			//cout << "chi: " << T[k] << endl;
			auto chi = boost::math::chi_squared(m - 1); // Chi-squared distribution with m-1 DoF
			double p = 1 - boost::math::cdf(chi, T[k]);
			double chiq = boost::math::quantile(chi, 1 - alpha);
			//cout << "quantile: " << chiq << endl;

			if (T[k] > chiq)
			{
				// Null hypothesis rejected -> do pairwise comparisons
				int best_id = 0;
				double best_rank = numeric_limits<double>::max();
				for (int i : ids) // Find best candidate = lowest rank sum
				{
					if (Rj[i] < best_rank)
					{
						best_id = i;
						best_rank = Rj[i];
					}
				}
				results << "Pairwise testing using best = " << best_id << endl;
				for (int h : ids) // Compare
				{
					if (h == best_id) continue;
					double snum = 2 * kf* (1 - (T[k] / (kf * (m - 1))));
					double score = abs(Rj[best_id] - Rj[h]) / sqrt((snum * Tden) / ((kf - 1) * (m - 1)));
					auto students = boost::math::students_t(m - 1);
					cout << "score: " << score << endl;
					double studq = boost::math::quantile(students, 1 - alpha / 2);
					cout << "quantile: " << studq << endl;

					if (score > studq)
					{
						results << h << " out of race: score = " << score << " / quantile = " << studq << endl;
						candidates.erase(h);
					}
				}
			}

			// Write block costs (fitness) to log
			results << "Block " << kf << " - chi = " << T[k] << " / chiq = " << chiq << endl;
			for (auto c : costs[k]) results << c.first << "; "; // Write IDs
			results << endl;
			for (auto c : costs[k]) results << c.second << "; "; // Write costs
			results << endl << endl;

			problem.reset(); // Get a new solution for a new block
			k++;
			//cin.get();
		}

		results.close();
	}
	else // Run with fixed parameters
	{
		double run_time = 60;
		unsigned int tabu_length = 100;
		bool large_neigh = true;
		unsigned int large_count = 100;
		unsigned int res_count = 3;

		auto algorithm = alg_type(problem, tabu_length, large_neigh, large_count, res_count);

		int iter = algorithm.evolve(run_time);

		problem.print_solution();

		cout << "Iterations: " << iter << endl;
	}

	cin.get();
	return 0;
}
