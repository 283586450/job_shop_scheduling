#include "algorithm.hpp"
#include "jobShopInstance.hpp"
#include "solution.hpp"
#include "types.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <random>
#include <vector>


namespace scheduling {

Individual GAAlgorithm::encode(const Solution& solution)
{
    Individual individual;
    individual.fitness = solution.makespan;
    individual.chromosome.reserve(solution.step_tasks_.size());

    // retrive the tasks from solution.step_tasks_ and store them in a vector
    std::vector<std::shared_ptr<StepTask>> tasks;
    tasks.reserve(solution.step_tasks_.size());
    for (const auto& [task_id, task] : solution.step_tasks_) {
        tasks.push_back(task);
    }

    // sort the tasks by start time
    std::sort(tasks.begin(), tasks.end(), [](const auto& lhs, const auto& rhs) {
        return lhs->start_time < rhs->start_time;
    });

    // retrive the job_id of each task in the sorted tasks, and push them into
    // the chromosome
    for (const auto& task : tasks) {
        individual.chromosome.push_back(task->job_id);
    }

    return individual;
}

Chromosome GAAlgorithm::encode(const JobShopInstance& instance)
{
    size_t total_steps = 0;
    for (const auto& job : instance.jobs()) {
        total_steps += job.second.steps.size();
    }

    Chromosome chromosome;
    chromosome.reserve(total_steps);

    for (const auto& [job_id, job_obj] : instance.jobs()) {
        //  add job_obj.steps.size() times the job_id to the chromosome
        std::fill_n(
            std::back_inserter(chromosome), job_obj.steps.size(), job_id);
    }

    //  shuffle the chromosome
    std::shuffle(chromosome.begin(),
                 chromosome.end(),
                 std::mt19937(std::random_device()()));

    return chromosome;
}

Solution GAAlgorithm::decode(const Chromosome&      chromosome,
                             const JobShopInstance& instance)
{
    Solution solution;   // solution obj to be returned

    std::map<MachineID, TimeStamp> machine_end_times;
    for (const auto& [machine_id, machine] : instance.machines()) {
        machine_end_times[machine_id] = 0;
    }

    std::map<JobID, StepID>    job_curr_step;
    std::map<JobID, TimeStamp> job_end_time;
    for (const auto& [job_id, job] : instance.jobs()) {
        job_curr_step[job_id] = 0;
        job_end_time[job_id]  = 0;
    }

    for (JobID job_id : chromosome) {
        StepID    step_id = job_curr_step[job_id];
        MachineID machine_id =
            instance.jobs().at(job_id).steps.at(step_id).machine_id;
        TimeStamp start_time =
            std::max(machine_end_times[machine_id], job_end_time[job_id]);
        TimeStamp end_time =
            start_time + instance.jobs().at(job_id).steps.at(step_id).duration;

        machine_end_times[machine_id] = end_time;
        job_end_time[job_id]          = end_time;
        job_curr_step[job_id]         = step_id + 1;

        solution.step_tasks_[{job_id, step_id}] = std::make_shared<StepTask>(
            machine_id,
            instance.jobs().at(job_id).steps.at(step_id).duration,
            start_time,
            end_time,
            job_id,
            step_id);

        solution.schedules_[machine_id].push_back(
            solution.step_tasks_[{job_id, step_id}]);

        solution.makespan = std::max(solution.makespan, end_time);
    }

    return solution;
}




}   // namespace scheduling
