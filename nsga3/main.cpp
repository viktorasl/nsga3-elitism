
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

using namespace std;

int main()
{
    // ---------- Step 14 / Algorithm 2 ----------

    ifstream exp_list("explist.ini");
    if (!exp_list) { cout << "We need the explist.ini file." << endl; return 1; }

    string exp_name;
    while (exp_list >> exp_name)
    {
        ifstream exp_ini("Experiments/" + exp_name);
        if (!exp_ini) { cout << exp_name << " file does not exist." << endl; continue; }

        // ----- Setup the expriment ------
        CNSGAIII nsgaiii(false);
        BProblem *problem = 0;

        SetupExperiment(nsgaiii, &problem, exp_ini);
        Gnuplot gplot;
        ofstream IGD_results(nsgaiii.name() + "-" + problem->name() + "-IGD.txt"); // output file for IGD values per run


        // ----- Run the algorithm to solve the designated function -----
        const size_t NumRuns = 20; // 20 is the setting in NSGA-III paper
        for (size_t r=0; r<NumRuns; r+=1)
        {
//            struct timespec start, end;

            srand(r); cout << "Solving " << problem->name() << " ... Run: " << r << endl;

            // --- Solve
            CPopulation solutions;

//            clock_gettime(CLOCK_MONOTONIC, &start);
            nsgaiii.Solve(&solutions, *problem);
//            clock_gettime(CLOCK_MONOTONIC, &end);
//            print_timediff("NSGA-III", start, end);

            // --- Output the result
            string logfname = "Results/" + nsgaiii.name() + "-" + problem->name() + "-Run" + IntToStr(r) + ".txt"; // e.g. NSGAIII-DTLZ1(3)-Run0.txt
            SaveScatterData(logfname, solutions);

            // --- Calculate the performance metric
            TFront PF, approximation;
            IGD_results << IGD(LoadFront(PF, "PF/"+ problem->name() + "-PF.txt"), LoadFront(approximation, logfname)) << endl;

            // --- Visualization (Show the last 3 dimensions. You need gnuplot.)
            ShowPopulation(gplot, solutions, "gnuplot-show"); //system("pause");
        }
        delete problem;

        //system("pause");

    }// while - there are more experiments to carry out

    return 0;
}
