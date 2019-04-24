#include "alg_nsgaiii.h"
#include "problem_base.h"
#include "alg_individual.h"
#include "alg_reference_point.h"
#include "alg_population.h"

#include "alg_initialization.h"
#include "alg_crossover.h"
#include "alg_mutation.h"
#include "alg_environmental_selection.h"
#include "alg_analysis.h"

#include "gnuplot_interface.h"
#include "log.h"

#include <vector>
#include <fstream>
#include <cmath>

using namespace std;

CNSGAIII::CNSGAIII():
	name_("NSGAIII"),
	gen_num_(1),
	pc_(1.0), // default setting in NSGA-III (IEEE tEC 2014)
	eta_c_(30), // default setting
	eta_m_(20) // default setting
{
}

// ----------------------------------------------------------------------
void CNSGAIII::Setup(ifstream &ifile)
{
	if (!ifile) return;

	string dummy;
	ifile >> dummy >> dummy >> name_;

	size_t p1 = 0, p2 = 0;
	ifile >> dummy >> dummy >> p1 >> p2;

	obj_division_p_.push_back(p1);

	if (!ifile) ifile.clear();
	else obj_division_p_.push_back(p2);
	
	ifile >> dummy >> dummy >> gen_num_;
	ifile >> dummy >> dummy >> pc_;
	ifile >> dummy >> dummy >> eta_c_;
	ifile >> dummy >> dummy >> eta_m_;
}
// ----------------------------------------------------------------------
void CNSGAIII::Solve(CPopulation *solutions, const BProblem &problem, bool improved_version)
{
	auto analysis = NSGAIIIAnalysis::None;
	CIndividual::SetTargetProblem(problem);
	
	vector<CReferencePoint> rps;
	GenerateReferencePoints(&rps, problem.num_objectives(), obj_division_p_); 
	size_t PopSize = rps.size();
	while (PopSize%4) PopSize += 1;

	CPopulation pop[2]={
		CPopulation(PopSize),
		CPopulation(PopSize)
	};
	CSimulatedBinaryCrossover SBX(pc_, eta_c_);
	CPolynomialMutation PolyMut(1.0/problem.num_variables(), eta_m_);

	Gnuplot gplot;

	int cur = 0, next = 1;
	RandomInitialization(&pop[cur], problem);
	for (size_t i=0; i<PopSize; i+=1)
	{
		problem.Evaluate(&pop[cur][i]);
	}
	
	size_t first_it_max_entropy = -1;
	size_t it_from_which_max_entropy = -1;
	const double max_entropy = log(rps.size());
	vector<CIndividual> elites(rps.size());
	vector<size_t> set_at(rps.size(), -1);
	std::vector<std::pair<size_t, double>> best_objs(pop[cur][0].objs().size(), make_pair(-1, numeric_limits<double>::max()));
	for (size_t t=0; t<gen_num_; t+=1)
	{
		pop[cur].resize(PopSize*2);

		for (size_t i=0; i<PopSize; i+=2)
		{
			int father = rand()%PopSize,
				mother = rand()%PopSize;

			SBX(&pop[cur][PopSize+i], &pop[cur][PopSize+i+1], pop[cur][father], pop[cur][mother]);

			PolyMut(&pop[cur][PopSize+i]);
			PolyMut(&pop[cur][PopSize+i+1]);

			problem.Evaluate(&pop[cur][PopSize+i]);
			problem.Evaluate(&pop[cur][PopSize+i+1]);
		}

		std::vector<int> rps_members;
		EnvironmentalSelection(t, &pop[next], &pop[cur], rps, elites, PopSize, improved_version, analysis, rps_members, set_at, best_objs);

		if (analysis & NSGAIIIAnalysis::Entropy)
		{
			double entropy = 0.0;
			for (auto members_count : rps_members) {
				if (members_count == 0)
				{
					continue;
				}
				double probability = (double)members_count / (double)rps_members.size();
				entropy -= probability * log(probability);
			}
			
			if (abs(max_entropy - entropy) < 10e-10)
			{
				if (first_it_max_entropy == -1)
				{
					first_it_max_entropy = t;
				}
				if (it_from_which_max_entropy == -1)
				{
					it_from_which_max_entropy = t;
				}
			}
			else
			{
				it_from_which_max_entropy = -1;
			}
			cout << entropy << endl;
		}
		//ShowPopulation(gplot, pop[next], "pop"); Sleep(50);

		std::swap(cur, next);
	}
	
	if (analysis & NSGAIIIAnalysis::Entropy)
	{
		cout << "Iterations: " << gen_num_ << endl;
		cout << "Max entropy: " << max_entropy << endl;
		cout << "First max entropy: " << first_it_max_entropy << endl;
		cout << "All max entropy: " << it_from_which_max_entropy << endl;
	}
	
	if (analysis & NSGAIIIAnalysis::ElitesUpdateTracking)
	{
		for (size_t i=0; i<set_at.size(); i+=1)
		{
			cout << i << " " << set_at[i] << endl;
		}
	}
	
	if (analysis & NSGAIIIAnalysis::ObjValIterationSetter)
	{
		for (size_t i=0; i<best_objs.size(); i+=1)
		{
			cout << i+1 << " " << best_objs[i].first << endl;
		}
	}

	*solutions = pop[cur];
}
