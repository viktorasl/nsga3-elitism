
#include "problem_base.h"
#include "alg_nsgaiii.h"
#include "alg_population.h"
#include "exp_experiment.h"

#include <ctime>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>

#include "gnuplot_interface.h"
#include "log.h"
#include "aux_misc.h"
#include "exp_indicator.h"
#include <numeric>
#include "aux_math.h"
#include <cmath>
#include <iomanip>

using namespace std;

template <typename It>
auto mean(It begin, It end)
{
    using T = typename iterator_traits<It>::value_type;
    auto size = (T)distance(begin, end);
    auto sum = accumulate(begin, end, (T)0);
    return sum / size;
}

template <typename It>
auto median(It begin, It end)
{
    using T = typename iterator_traits<It>::value_type;
    vector<T> data(begin, end);
    nth_element(data.begin(), data.begin() + data.size() / 2, data.end());
    return data[data.size() / 2];
}

auto stddev(vector<double> &inp)
{
    auto mean_val = mean(inp.begin(), inp.end());
    double sq_sum = 0.0;
    for (auto el : inp)
    {
        sq_sum += MathAux::square(el - mean_val);
    }
    return sqrt(sq_sum) / (double)inp.size();
}

void print_analysis_result(const string& metrics, double value)
{
    cout << left << setw(8) << setfill(' ') << metrics << ": " << value << endl;
}

int main()
{
    // ---------- Step 14 / Algorithm 2 ----------

    ifstream exp_list("explist.ini");
    if (!exp_list) { cout << "We need the explist.ini file." << endl; return 1; }

    string exp_name;
    string improved_prefix = "IMP_";
    while (exp_list >> exp_name)
    {
        bool is_improved_version = false;
        if (!exp_name.compare(0, improved_prefix.size(), improved_prefix))
        {
            is_improved_version = true;
            exp_name.erase(0, improved_prefix.size());
        }
        ifstream exp_ini("Experiments/" + exp_name);
        if (!exp_ini) { cout << exp_name << " file does not exist." << endl; continue; }

        // ----- Setup the expriment ------
        CNSGAIII nsgaiii;
        BProblem *problem = 0;

        SetupExperiment(nsgaiii, &problem, exp_ini);
        Gnuplot gplot;
        string out_problem_name = (is_improved_version ? improved_prefix : "") + problem->name();
        ofstream IGD_results(nsgaiii.name() + "-" + out_problem_name + "-IGD.txt"); // output file for IGD values per run


        // ----- Run the algorithm to solve the designated function -----
        
        vector<double> igd_values;
        const size_t NumRuns = 20; // 20 is the setting in NSGA-III paper
        cout << "Solving " << problem->name() << (is_improved_version ? "(w/ improved algo)" : "") << endl;
        for (size_t r=0; r<NumRuns; r+=1)
        {
//            struct timespec start, end;
#if VERBOSE_RUNS
            srand(r); cout << "...Run: " << r << endl;
#endif
            // --- Solve
            CPopulation solutions;

//            clock_gettime(CLOCK_MONOTONIC, &start);
            nsgaiii.Solve(&solutions, *problem, is_improved_version);
//            clock_gettime(CLOCK_MONOTONIC, &end);
//            print_timediff("NSGA-III", start, end);

            // --- Output the result
            string logfname = "Results/" + nsgaiii.name() + "-" + out_problem_name + "-Run" + IntToStr(r) + ".txt"; // e.g. NSGAIII-DTLZ1(3)-Run0.txt
            SaveScatterData(logfname, solutions);

            // --- Calculate the performance metric
            TFront PF, approximation;
            double igd_value = IGD(LoadFront(PF, "PF/"+ problem->name() + "-PF.txt"), LoadFront(approximation, logfname));
			cout << igd_value << endl;
            igd_values.push_back(igd_value);
            IGD_results << igd_value << endl;

            // --- Visualization (Show the last 3 dimensions. You need gnuplot.)
            ShowPopulation(gplot, solutions, "gnuplot-show"); //system("pause");
        }
        delete problem;
        
        auto minmax_val = minmax_element(igd_values.begin(), igd_values.end());
        auto median_val = median(igd_values.begin(), igd_values.end());
        auto mean_val = mean(igd_values.begin(), igd_values.end());
        auto stddev_val = stddev(igd_values);
        
        print_analysis_result("Min", *minmax_val.first);
        print_analysis_result("Median", median_val);
        print_analysis_result("Max", *minmax_val.second);
        print_analysis_result("Mean", mean_val);
        print_analysis_result("Std Dev", stddev_val);
        
        //system("pause");

    }// while - there are more experiments to carry out

    return 0;
}
