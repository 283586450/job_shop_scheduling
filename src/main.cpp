#include <iostream>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"

#include "jobShopInstance.hpp"
#include "run.hpp"

ABSL_FLAG(int, num_jobs, 10, "Number of jobs for the test instance");
ABSL_FLAG(int, num_machines, 10, "Number of machines for the test instance");
ABSL_FLAG(int, num_threads, 4, "Number of threads for solving process");
ABSL_FLAG(int, time_limit, 10, "Time limit of solving process");


int main(int argc, char** argv)
{
  
  std::cout << "hellow world! \n";

                 absl::SetProgramUsageMessage(
        "A program to solve job shop scheduling problem with multi-algorithms "
        "and multi-threads.\n"
        "Usage:\n"
        "    ./job_shop_scheduling --num_jobs=<int> --num_machines=<int> \n "
        "Options:\n"
        "    -num_jobs   The number of jobs for the test instance. \n"
        "    -num_machines   The number of machines for the test instance. ");
    absl::ParseCommandLine(argc, argv);

    int num_jobs     = absl::GetFlag(FLAGS_num_jobs);   // Fixed flag name
    int num_machines = absl::GetFlag(FLAGS_num_machines);
    int num_threads  = absl::GetFlag(FLAGS_num_threads);
    int time_limit   = absl::GetFlag(FLAGS_time_limit);

    std::cout << "num of jobs: " << num_jobs
              << ", num of machines: " << num_machines << "\n";

    scheduling::JobShopInstance instance;
    instance.generate_instance(num_jobs, num_machines);
    instance.print();

    // scheduling::SolutionConstructor solution_constructor;
    // solution_constructor.convert_from_instance(instance);
    // scheduling::Solution solution = solution_constructor.schedule();
    // solution.print();
    scheduling::Run run{instance, num_threads, time_limit};
    run();

    return 0;
}

