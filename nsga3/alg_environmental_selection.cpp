#include "alg_environmental_selection.h"
#include "alg_population.h"
#include "aux_math.h"
#include "alg_nondominated_sort.h"
#include "alg_reference_point.h"

#include <limits>
#include <algorithm>
#include <iostream>
#include <cmath>

using namespace std;

// ----------------------------------------------------------------------
// TranslateObjectives():
//
// 1. Find the ideal point
// 2. Translate the objective values
// 3. Return the ideal point
//
// Check steps 1-3 in Algorithm 2 in the original paper of NSGAIII.
// ----------------------------------------------------------------------
vector<double> TranslateObjectives(CPopulation *ppop, const CNondominatedSort::TFronts &fronts)
{
	CPopulation &pop = *ppop;
	vector<double> ideal_point(pop[0].objs().size());

	const size_t NumObj = pop[0].objs().size();
	for (size_t f=0; f<NumObj; f+=1)
	{
		double minf = numeric_limits<double>::max();
		for (size_t i=0; i<fronts[0].size(); i+=1) // min values must appear in the first front
		{
			minf = std::min(minf, pop[ fronts[0][i] ].objs()[f]);
		}
		ideal_point[f] = minf;

		for (size_t t=0; t<fronts.size(); t+=1)
		{
			for (size_t i=0; i<fronts[t].size(); i+=1)
			{
				size_t ind = fronts[t][i];
				pop[ind].conv_objs().resize(NumObj);
				pop[ind].conv_objs()[f] = pop[ind].objs()[f] - minf;
			}
		}
	}

	return ideal_point;

}// TranslateObjectives()

// ----------------------------------------------------------------------
// FindExtremePoints():
//
// Find the extreme points along each objective axis.
// The extreme point has the minimal ASF value.
// Return the indices of extreme individuals in the population.
//
// Check step 4 in Algorithm 2 and eq. (4) in the original paper.
// ----------------------------------------------------------------------
void FindExtremePoints(vector<size_t> *extreme_points, const CPopulation &pop, const CNondominatedSort::TFronts &fronts)
{
	vector<size_t> &exp = *extreme_points;
	exp.clear();

	for (size_t f=0; f<pop[0].objs().size(); f+=1)
	{
		vector<double> w(pop[0].objs().size(), 0.000001);
		w[f] = 1.0;

		double min_ASF = numeric_limits<double>::max();
		size_t min_indv = fronts[0].size();

		for (size_t i=0; i<fronts[0].size(); i+=1)  // only consider the individuals in the first front
		{
			double asf = MathAux::ASF(pop[ fronts[0][i] ].conv_objs(), w); // nsga3cpp 1.11 (2015.04.26 thanks to Vivek Nair for his correction by email)

			if ( asf < min_ASF )
			{
				min_ASF = asf;
				min_indv = fronts[0][i];
			}
		}

		exp.push_back(min_indv);
	}

}// FindExtremePoints()


// ----------------------------------------------------------------------
// FindMaxObjectives(): (added in nsga3cpp v1.2)
//
// Find the maximal objective values among the current solutions.
// Intercepts are set by these values when we cannot construct the
// hyperplane.
//
// This method follows the implementation in the following paper:
//
// Yuan Yuan, Hua Xu, Bo Wang,
// "An Experimental Investigation of Variation Operators in
//  Reference-Point Based Many-Objective Optimization,"
// GECCO, pp. 775-782, 2015
//
// I think Jerry Lee for finding out the difference between the two
// implementations and recommending the modification.
//
// ----------------------------------------------------------------------
vector<double> FindMaxObjectives(const CPopulation &pop)
{
	const size_t NumObj = pop[0].objs().size();

	vector<double> max_point(NumObj, -numeric_limits<double>::max());
	for (size_t i=0; i<pop.size(); i+=1)
	{
		for (size_t f=0; f<NumObj; f+=1)
		{
			max_point[f] = std::max(max_point[f], pop[i].objs()[f]);
		}
	}

	return max_point;
}

// ----------------------------------------------------------------------
// ConstructHyperplane():
//
// Given the extreme points, construct the hyperplane.
// Then, calculate the intercepts.
//
// Check step 6 in Algorithm 2 in the original paper.
// ----------------------------------------------------------------------
void ConstructHyperplane(vector<double> *pintercepts, const CPopulation &pop, const vector<size_t> &extreme_points)
{
	// Check whether there are duplicate extreme points.
	// This might happen but the original paper does not mention how to deal with it.
	bool duplicate = false;
	for (size_t i=0; !duplicate && i<extreme_points.size(); i+=1)
	{
		for (size_t j=i+1; !duplicate && j<extreme_points.size(); j+=1)
		{
			duplicate = (extreme_points[i] == extreme_points[j]);
		}
	}

	vector<double> &intercepts = *pintercepts;
	intercepts.assign(pop[0].objs().size(), 0);

	bool negative_intercept = false;
	if (!duplicate)
	{
		// Find the equation of the hyperplane
		vector<double> b(pop[0].objs().size(), 1.0);
		vector< vector<double> > A;
		for (size_t p=0; p<extreme_points.size(); p+=1)
		{
			A.push_back(pop[ extreme_points[p] ].conv_objs()); // v1.11: objs() -> conv_objs()
		}
		vector<double> x;
		MathAux::GuassianElimination(&x, A, b);

		// Find intercepts
		for (size_t f=0; f<intercepts.size(); f+=1)
		{
			intercepts[f] = 1.0/x[f];

			if(x[f] < 0)
			{
				negative_intercept = true;
				break;
			}
		}
	}

	if (duplicate || negative_intercept) // v1.2: follow the method in Yuan et al. (GECCO 2015)
	{
		vector<double> max_objs = FindMaxObjectives(pop);
		for (size_t f=0; f<intercepts.size(); f+=1)
		{
			intercepts[f] = max_objs[f];
		}
	}
}

// ----------------------------------------------------------------------
// NormalizeObjectives():
//
// Normalize objective values with respect to the intercepts and the ideal point.
// Check step  7 in Algorithm 2 and eq. (5) in the original paper.
// ----------------------------------------------------------------------
void NormalizeObjectives(CPopulation *ppop, const CNondominatedSort::TFronts &fronts, const vector<double> &intercepts, const vector<double> &ideal_point)
{
	CPopulation &pop = *ppop;

	for (size_t t=0; t<fronts.size(); t+=1)
	{
		for (size_t i=0; i<fronts[t].size(); i+=1)
		{
			size_t ind = fronts[t][i];
			for (size_t f=0; f<pop[ ind ].conv_objs().size(); f+=1)
			{
				if ( fabs(intercepts[f])>10e-10 ) // avoid the divide-by-zero error
					pop[ ind ].conv_objs()[f] = pop[ ind ].conv_objs()[f]/(intercepts[f]); // v1.11: fixed
				else
					pop[ ind ].conv_objs()[f] = pop[ ind ].conv_objs()[f]/10e-10;
			}
		}
	}

}// NormalizeObjectives()

// ----------------------------------------------------------------------
// FindNicheReferencePoint():
//
// Find the reference point with the minimal cluster size.
// Return one randomly if there is more than one point.
//
// Check steps 3-4 in Algorithm 4 in the original paper.
// ----------------------------------------------------------------------
size_t FindNicheReferencePoint(const vector<CReferencePoint> &rps)
{
	// find the minimal cluster size
	size_t min_size = numeric_limits<size_t>::max();
	for (size_t r=0; r<rps.size(); r+=1)
	{
		min_size = std::min(min_size, rps[r].MemberSize());
	}

	// find the reference points with the minimal cluster size Jmin
	vector<size_t> min_rps;
	for (size_t r=0; r<rps.size(); r+=1)
	{
		if (rps[r].MemberSize() == min_size)
		{
			min_rps.push_back(r);
		}
	}

	// return a random reference point (j-bar)
	return min_rps[rand()%min_rps.size()];
}

// ----------------------------------------------------------------------
// SelectClusterMember():
//
// Select a potential member (an individual in the front Fl) and associate
// it with the reference point.
//
// Check the last two paragraphs in Section IV-E in the original paper.
// ----------------------------------------------------------------------
int SelectClusterMember(const CReferencePoint &rp)
{
	int chosen = -1;
	if (rp.HasPotentialMember())
	{
		if (rp.MemberSize() == 0) // currently has no member
		{
			chosen =  rp.FindClosestMember();
		}
		else
		{
			chosen =  rp.RandomMember();
		}
	}
	return chosen;
}

void AppendPopulationMember(CPopulation& next, CIndividual& indv, NSGAIIIAnalysis analysis, size_t t, vector<pair<size_t, double>>& best_objs)
{
	next.push_back(indv);
	if (analysis & NSGAIIIAnalysis::ObjValIterationSetter)
	{
		for (size_t i=0; i<indv.objs().size(); i+=1)
		{
			if (indv.objs()[i] < best_objs[i].second)
			{
				best_objs[i] = make_pair(t, indv.objs()[i]);
			}
		}
	}
}

// ----------------------------------------------------------------------
// EnvironmentalSelection():
//
// Check Algorithms 1-4 in the original paper.
// ----------------------------------------------------------------------
void EnvironmentalSelection(size_t t, CPopulation *pnext, CPopulation *pcur, vector<CReferencePoint> rps, vector<CIndividual>& elites, size_t PopSize, bool improved_version, NSGAIIIAnalysis analysis, vector<int>& rps_members, vector<size_t>& set_at, vector<pair<size_t, double>>& best_objs)
{
	CPopulation &cur = *pcur, &next = *pnext;
	next.clear();

	// ---------- Step 4 in Algorithm 1: non-dominated sorting ----------
	CNondominatedSort::TFronts fronts = NondominatedSort(cur);

	// ---------- Steps 5-7 in Algorithm 1 ----------
	vector<size_t> considered; // St
	size_t last = 0, next_size = 0;
	while (next_size < PopSize)
	{
		next_size += fronts[last].size();
		last += 1;
	}
	fronts.erase(fronts.begin()+last, fronts.end()); // remove useless individuals

	for (size_t t=0; t<fronts.size()-1; t+=1)
	{
		for (size_t i=0; i<fronts[t].size(); i+=1)
		{
			AppendPopulationMember(next, cur[ fronts[t][i] ], analysis, t, best_objs);
		}
	}

	// ---------- Steps 9-10 in Algorithm 1 ----------
	if (next.size() == PopSize) return;

	// ---------- Step 14 / Algorithm 2 ----------
	vector<double> ideal_point = TranslateObjectives(&cur, fronts);

	vector<size_t> extreme_points;
	FindExtremePoints(&extreme_points, cur, fronts);

	vector<double> intercepts;
	ConstructHyperplane(&intercepts, cur, extreme_points);

	NormalizeObjectives(&cur, fronts, intercepts, ideal_point);

	// ---------- Step 15 / Algorithm 3, Step 16 ----------
	Associate(&rps, cur, fronts);

	// ---------- Step 17 / Algorithm 4 ----------
	size_t next_rp = 0;
	
	vector<size_t> rps_indices;
	for (size_t i=0; i<rps.size(); i+=1)
	{
		rps_indices.push_back(i);
	}
	if (analysis & NSGAIIIAnalysis::Entropy)
	{
		rps_members.resize(rps.size(), 0);
	}
	
	size_t elites_used = 0;
	size_t elites_updated = 0;
	while (next.size() < PopSize)
	{
		size_t min_rp = 0;
		if (fronts.size() == 1 && improved_version && next_rp < rps.size())
		{
			min_rp = next_rp;
		} else {
			min_rp = FindNicheReferencePoint(rps);
		}
		auto pt_rp_idx = rps_indices[min_rp];
		
		int chosen = SelectClusterMember(rps[min_rp]);
		if (chosen < 0) // no potential member in Fl, disregard this reference point
		{
			rps.erase(rps.begin()+min_rp);
			rps_indices.erase(rps_indices.begin()+min_rp);
		}
		else
		{
			if (analysis & NSGAIIIAnalysis::Entropy)
			{
				rps_members[pt_rp_idx] = (rps_members[pt_rp_idx] + 1);
			}
			
			auto chosen_member = cur[chosen];
			auto elite = elites[pt_rp_idx];
			if (elite.vars()[0] != 0)
			{
				double elite_length = MathAux::length(elite.objs());
				double member_length = MathAux::length(chosen_member.objs());
				
				double elite_dst = MathAux::PerpendicularDistance(rps[min_rp].pos(), elite.objs());
				double member_dst = MathAux::PerpendicularDistance(rps[min_rp].pos(), chosen_member.objs());
				
				bool ignore = rand() % 2;
				
				bool new_is_better = ((member_length < elite_length) || (member_dst < elite_dst));
				if (new_is_better)
				{
					AppendPopulationMember(next, chosen_member, analysis, t, best_objs);
					if (analysis & NSGAIIIAnalysis::ElitesUpdateTracking)
					{
						set_at[pt_rp_idx] = t;
					}
				}
				else
				{
					AppendPopulationMember(next, elite, analysis, t, best_objs);
					elites_used++;
				}
				
				// elite preservation natural selection member advantage coeficient
				float mmb_adv = (rand() % 2 == 0) ? 1.1 : 1.3;
				if (member_length < elite_length * mmb_adv && member_dst < elite_dst * mmb_adv)
				{
					elites[pt_rp_idx] = chosen_member;
					elites_updated++;
				}
			}
			else
			{
				elites[pt_rp_idx] = chosen_member;
				AppendPopulationMember(next, chosen_member, analysis, t, best_objs);
			}
			
			rps[min_rp].AddMember();
			rps[min_rp].RemovePotentialMember(chosen);
			next_rp+=1;
		}
	}
	
	if (analysis & NSGAIIIAnalysis::ElitesUsedAndUpdated)
	{
		cout << elites_used << " " << elites_updated << endl;
	}
}
// ----------------------------------------------------------------------
