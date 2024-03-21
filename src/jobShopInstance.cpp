#include <algorithm>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

#include "jobShopInstance.hpp"

namespace scheduling {

void JobShopInstance::generate_instance(int num_job, int num_machine)
{
    std::random_device rnd_dev;
    std::mt19937       rnd_engine(rnd_dev());

    // add machines
    for (int i = 0; i < num_machine; i++) {
        add_machine(i);
    }

    // add jobs
    const int                          LB_JOB_DUE_DATE = num_job * num_machine;
    const int                          UB_JOB_DUE_DATE = LB_JOB_DUE_DATE + 50;
    std::uniform_int_distribution<int> job_due_distro(LB_JOB_DUE_DATE,
                                                      UB_JOB_DUE_DATE);
    for (int i = 0; i < num_job; i++) {
        add_job(i, job_due_distro(rnd_engine));
    }

    // add steps
    const int                          LB_STEP_DURATION = 3;
    const int                          UB_STEP_DURATION = 10;
    std::uniform_int_distribution<int> step_duration_distro(LB_STEP_DURATION,
                                                            UB_STEP_DURATION);
    for (int i = 0; i < num_job; i++) {
        std::vector<int> machine_list(num_machine);
        std::iota(machine_list.begin(), machine_list.end(), 0);
        std::shuffle(machine_list.begin(), machine_list.end(), rnd_engine);

        for (int j = 0; j < num_machine; j++) {
            add_step(i, j, machine_list[j], step_duration_distro(rnd_engine));
        }
    }
}

void JobShopInstance::print() const
{
    for (const auto& [job_id, job] : jobs_) {
        std::cout << "Job " << job_id << " due date: " << job.due_date << '\n';
        for (const auto& [step_id, step] : job.steps) {
            std::cout << "Step " << step_id << " on machine " << step.machine_id
                      << " duration: " << step.duration << '\n';
        }
    }
    for (const auto& [machine_id, machine] : machines_) {
        std::cout << "Machine " << machine_id << '\n';
    }
}

}   // namespace scheduling