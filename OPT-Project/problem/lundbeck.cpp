#include "lundbeck.h"
#include <iostream>
#include <fstream>
#include <boost/lexical_cast.hpp>
#include <list>

#ifdef _WIN32
#include <ppl.h>
#else
#include "tbb/parallel_for.h"
#include "tbb/combinable.h"
#endif

namespace problem
{
	lundbeck::lundbeck(std::string file, unsigned int machines) : n_machines(machines)
	{
		using namespace std;
		using namespace boost;

		machine_dist = uniform_int_distribution<int>(0, n_machines - 1);
		rand.seed((unsigned long)time(0));

		for (unsigned int i = 0; i < machines; i++)
		{
			vector<unsigned int> j;
			solution.push_back(j);
		}

		string buf;
		ifstream f(file);
		if (!f.is_open())
		{
			cout << "Error opening problem file.";
			cin.get();
			exit(1);
		}

		n_jobs = 0;
		while (!f.eof()) // Loop for each job
		{
			try
			{
				n_jobs++;
				job j;

				// MRP ctrlr
				getline(f, buf, ',');

				// Order ID
				getline(f, buf, ',');
				j.id = lexical_cast<unsigned int>(buf);

				// Material
				getline(f, buf, ',');
				j.material = lexical_cast<unsigned int>(buf);

				// Prod. line
				getline(f, buf, ',');
				if (buf == "640-2")
				{
					j.machine_type = job::machinetype::ONETWO;
				}
				else if (buf == "640-8-1")
				{
					j.machine_type = job::machinetype::ALL;
				}
				else if (buf == "")
				{
					j.machine_type = job::machinetype::ALL;
				}

				// Height
				getline(f, buf, ',');
				j.height = lexical_cast<unsigned int>(buf);

				// Leaflet
				getline(f, buf, ',');
				j.leaflet = lexical_cast<unsigned int>(buf);

				// Type of foil
				getline(f, buf, ',');

				// Formulation
				getline(f, buf, ',');
				if (buf == "Tablets - filmcoated")
				{
					j.formulation = 0;
				}
				else if (buf == "Tablets - sugarcoated")
				{
					j.formulation = 1;
				}

				// Hotmelt
				getline(f, buf, ',');
				if (buf == "NO")
				{
					j.hotmelt = false;
				}
				else
				{
					j.hotmelt = true;
					j.machine_type = job::machinetype::TWO;
				}

				// Matrix bar
				getline(f, buf, ',');

				// Unit dose
				getline(f, buf, ',');

				// Order dev
				getline(f, buf, ',');

				// Country
				getline(f, buf, ',');

				// Pre numb.
				getline(f, buf, ',');
				j.prenumb = lexical_cast<unsigned int>(buf);

				// BOM
				getline(f, buf, ',');

				// Strength
				getline(f, buf, ',');
				j.strength = lexical_cast<double>(buf);

				// Blisters
				getline(f, buf, ',');
				j.blisters = lexical_cast<unsigned int>(buf);

				// Packing type
				getline(f, buf, ',');
				j.packing_type = buf;
				if (buf == "Unit dose")
				{
					j.machine_type = job::machinetype::ONE;
				}

				// Target qty
				getline(f, buf, ',');

				// Blister
				getline(f, buf, ',');

				// Bsc start
				getline(f, buf, ',');

				// Nummeret
				getline(f, buf, ',');
				j.nummeret = lexical_cast<unsigned int>(buf);

				// DOS
				getline(f, buf, ',');

				// GR
				getline(f, buf, ',');

				// Total DOS
				getline(f, buf, ',');
				j.dos = lexical_cast<unsigned int>(buf);

				// MDVP
				getline(f, buf);
				j.mdvp = lexical_cast<unsigned int>(buf);

				// SKIP HALF THE JOBS
				getline(f, buf);

				// Add job to jobs map
				jobs[j.id] = j;

				// Add job to initial solution
				add_initial_solution(j);
			}
			catch (boost::bad_lexical_cast e)
			{
				std::cout << e.what() << ": " << buf << std::endl;
				cin.get();
				exit(1);
			}

		}

		f.close();
	}

	lundbeck::~lundbeck()
	{
	}

	// Calculate cleaning time between jobs
	double clean_time(const job& from_job, const job& to_job)
	{
		double time = 0;
		if (from_job.material == to_job.material) return 50; // Same material number
		
		if (!from_job.hotmelt && to_job.hotmelt || from_job.hotmelt && !to_job.hotmelt) time += 60;

		if (from_job.packing_type == to_job.packing_type) time += 120;

		if (from_job.prenumb == to_job.prenumb)
		{
			if (from_job.strength == to_job.strength)
			{
				if (from_job.height == to_job.height)
				{
					return time + 50;
				}
				else
				{
					return time + 100;
				}
			}
			else
			{
				if (from_job.height == to_job.height)
				{
					return time + 75;
				}
				else
				{
					return time + 140;
				}
			}
		}
		else
		{
			if (from_job.height == to_job.height)
			{
				return time + 80;
			}
			else
			{
				return time + 160;
			}
		}
		return 60;
	}
	lundbeck::fitness_type lundbeck::fitness(const solution_type& s)
	{
		// The fitness is the makespan (plus DOS penalty if not valid solution)
		double fitness = 0;
		for (auto machine_it = s.begin(); machine_it != s.end(); machine_it++)
		{
			double this_fitness = 0;
			double this_time = 0;
			auto& machine_list = *machine_it;
			auto from_job_it = machine_list.begin();
			for (auto job_it = machine_list.begin()++; job_it != machine_list.end(); job_it++)
			{
				auto to_job_it = job_it;
				const job& from_job = jobs[*from_job_it];
				const job& to_job = jobs[*to_job_it];

				auto time = from_job.nummeret + clean_time(from_job, to_job);
				this_time += time;
				this_fitness += time;
				// Penalize DOS
				if (from_job.dos == 0 && this_time > 4320) // 4320 minutes = 72 hours 
				{
					this_fitness += 500; // Set a relatively big penalty
				}

				from_job_it = to_job_it;
			}
			this_fitness += jobs[*from_job_it].nummeret; // Last job must also be added

			// Penalize DOS
			if (jobs[*from_job_it].dos == 0 && this_time > 4320) // 4320 minutes = 72 hours 
			{
				this_fitness += 500; // Set a relatively big penalty
			}

			if (this_fitness > fitness) fitness = this_fitness; // Choose the highest makespan from all machines
		}
		return fitness;
	}

	bool lundbeck::is_valid(const lundbeck::solution_type& sol)
	{
		// TODO: Make sure everything is checked for
		for (unsigned int m = 0; m < n_machines; m++)
		{
			for (int j = 0, jmax = (int)sol[m].size(); j < jmax; j++)
			{
				// Check machine types
				if (m == 0 && jobs[sol[m][j]].machine_type == job::machinetype::TWO) return false;
				if (m == 1 && jobs[sol[m][j]].machine_type == job::machinetype::ONE) return false;
				if (m == 2 && jobs[sol[m][j]].machine_type != job::machinetype::ALL) return false;
			}
		}
		return true;
	}

	void lundbeck::move_between(std::vector<unsigned int>& f, std::vector<unsigned int>& t, int from, int to)
	{
		// Doesn't work, but would maybe be faster...
		//std::move(f.begin() + from, f.begin() + from, t.begin() + to);
		//f.erase(f.begin() + from);

		// Alternative
		auto el = f.at(from);
		f.erase(f.begin() + from);
		t.insert(t.begin() + to, el);
	}

	void lundbeck::find_neigh_thread(std::vector<solution_type>& neighbours, int size)
	{
		//for (int fm = 0; fm < 3; fm++)
#ifdef _WIN32
		using namespace Concurrency;
#else
		using namespace tbb;
#endif

		combinable<std::vector<solution_type>> n_combinable;
		parallel_for((const unsigned int)0, n_machines, [&n_combinable, size, this](int fm) // From machine
		{
			solution_type tmp_sol(solution);
			for (int tm = 0, tmmax = n_machines; tm < tmmax; tm++) // To machine
			{
				for (int fj = 0, fjmax = (int)solution[fm].size(); fj < fjmax; fj++) // From job
				{
					for (int tj = 0, tjmax = (int)solution[tm].size(); tj < tjmax; tj++) // To job
					{
						if (fj == tj) continue;

						move_between(tmp_sol[fm], tmp_sol[tm], fj, tj);

						if (is_valid(tmp_sol))
						{
							n_combinable.local().push_back(tmp_sol);
						}

						if (size == 2) // This makes neighbourhood size = 2 (danger)
						{
							solution_type tmp_sol2 = tmp_sol;
							for (int fm2 = 0, fmmax2 = n_machines; fm2 < fmmax2; fm2++)
							{
								for (int tm2 = 0, tmmax2 = n_machines; tm2 < tmmax2; tm2++) // To machine
								{
									for (int fj2 = 0, fjmax2 = (int)tmp_sol[fm2].size(); fj2 < fjmax2; fj2++) // From job
									{
										for (int tj2 = 0, tjmax2 = (int)tmp_sol[tm2].size(); tj2 < tjmax2; tj2++) // To job
										{
											if (fj2 == tj2) continue;

											move_between(tmp_sol2[fm2], tmp_sol2[tm2], fj2, tj2);

											if (is_valid(tmp_sol2))
											{
												n_combinable.local().push_back(tmp_sol2);
											}

											// Move back again
											move_between(tmp_sol2[tm2], tmp_sol2[fm2], tj2, fj2);

										}
									}
								}
							}
						}

						// Move back again
						move_between(tmp_sol[tm], tmp_sol[fm], tj, fj);

					}
				}
			}
		});
		// Re-combine neighbour solutions
		n_combinable.combine_each([&neighbours](const std::vector<solution_type>& vec)
		{
			neighbours.insert(neighbours.begin(), vec.cbegin(), vec.cend());
			//std::copy(vec.begin(), vec.end(), std::back_inserter(neighbours));
		});
	}
	// Get all neighbours for solution 's' within a neighbourhood of size 'size'
	std::vector<lundbeck::solution_type> lundbeck::neighbours(int size) // With move-type neighbourhood, size can ONLY be 1 - scales as (j - 1)^2n
	{
		std::vector<solution_type> neighbours, neighbours1, neighbours2, neighbours3;

		find_neigh_thread(neighbours, size);

		return neighbours;
	}

	void lundbeck::add_initial_solution(job& j)
	{
		if (j.machine_type == job::machinetype::ALL)
		{
			// Insert on a random machine at a random position
			std::vector<unsigned int>& m = solution[machine_dist(rand)];
			if (m.size() == 0)
			{
				m.push_back(j.id);
				return;
			}
			auto job_dist = std::uniform_int_distribution<int>(0, (int)m.size() - 1);
			auto it = m.begin();
			std::advance(it, job_dist(rand));
			m.insert(it, j.id);

			return;
		}
		if (j.machine_type == job::machinetype::ONE)
		{
			// Insert on first machine at a random position
			std::vector<unsigned int>& m = solution[0];
			if (m.size() == 0)
			{
				m.push_back(j.id);
				return;
			}
			auto job_dist = std::uniform_int_distribution<int>(0, (int)m.size() - 1);
			auto it = m.begin();
			std::advance(it, job_dist(rand));
			m.insert(it, j.id);

			return;
		}
		if (j.machine_type == job::machinetype::TWO)
		{
			// Insert on second machine at a random position
			std::vector<unsigned int>& m = solution[1];
			if (m.size() == 0)
			{
				m.push_back(j.id);
				return;
			}
			auto job_dist = std::uniform_int_distribution<int>(0, (int)m.size() - 1);
			auto it = m.begin();
			std::advance(it, job_dist(rand));
			m.insert(it, j.id);

			return;
		}
		if (j.machine_type == job::machinetype::ONETWO)
		{
			// Insert on first or second machine at a random position
			auto m_dist = std::uniform_int_distribution<int>(0, 1);
			std::vector<unsigned int>& m = solution[m_dist(rand)];
			if (m.size() == 0)
			{
				m.push_back(j.id);
				return;
			}
			auto job_dist = std::uniform_int_distribution<int>(0,  (int)m.size() - 1);
			auto it = m.begin();
			std::advance(it, job_dist(rand));
			m.insert(it, j.id);

			return;
		}
		
	}

	void print(std::ostream& os, const std::vector<unsigned int>& s) {
		for (auto i = s.begin(); i != s.end(); ++i)
			os << *i << " ";
		os << std::endl;
	}
	void print(std::ostream& os, const lundbeck::solution_type& A)
	{
		int count = 1;
		for (auto i = A.begin(); i != A.end(); ++i)
		{
			os << "Machine " << count << std::endl;
			print(os, *i);
			count++;
		}
	}
	void lundbeck::print_solution()
	{
		std::cout << "Solution: " << std::endl;
		print(std::cout, solution);
		std::cout << "Fitness: " << fitness(solution) << std::endl;
	}
}
