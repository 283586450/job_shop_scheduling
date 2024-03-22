#include "algorithm.hpp"
#include "jobShopInstance.hpp"
#include "solution.hpp"
#include "types.hpp"

#include <algorithm>
#include <cstddef>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <utility>
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

Individual GAAlgorithm::encode(const JobShopInstance& instance)
{
    size_t total_steps = 0;
    for (const auto& job : instance.jobs()) {
        total_steps += job.second.steps.size();
    }

    Individual individual;
    individual.chromosome.reserve(total_steps);


    for (const auto& [job_id, job_obj] : instance.jobs()) {
        //  add job_obj.steps.size() times the job_id to the chromosome
        std::fill_n(std::back_inserter(individual.chromosome),
                    job_obj.steps.size(),
                    job_id);
    }

    //  shuffle the chromosome
    std::shuffle(individual.chromosome.begin(),
                 individual.chromosome.end(),
                 std::mt19937(std::random_device()()));

    Solution sol       = decode(individual.chromosome, instance);
    individual.fitness = sol.makespan;
    return individual;
}

Solution GAAlgorithm::decode(const Chromosome&      chromosome,
                             const JobShopInstance& instance)
{
    Solution     solution{};   // solution obj to be returned
    unsigned int makespan = 0;

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

        makespan = std::max(makespan, end_time);
    }

    solution.chromo_  = chromosome;
    solution.makespan = makespan;

    return solution;
}

Individual GAAlgorithm::tournament_selection(const Population& population)
{
    std::vector<Individual> tournament;
    tournament.reserve(5);

    std::random_device                    rd;
    std::mt19937                          rng(rd());
    std::uniform_int_distribution<size_t> dist(0, population.size() - 1);

    for (int i = 0; i < 5; i++) {
        tournament.push_back(population[dist(rng)]);
    }
    auto best = std::min_element(tournament.begin(),
                                 tournament.end(),
                                 [](const auto& lhs, const auto& rhs) {
                                     return lhs.fitness < rhs.fitness;
                                 });
    return *best;
}


std::pair<Chromosome, Chromosome> GAAlgorithm::crossover(
    const Individual& parent1, const Individual& parent2)
{
    // split jobs into two group randomly [groupA, groupB],
    // for child 1, keep the position of groupA in parent1, and keep the
    // sequence of groupB in parent2 for child 2, keep the position of groupB in
    // parent2, and keep the sequence of groupA in parent1
    JobID              max_job_id = std::ranges::max(parent1.chromosome);
    std::random_device rd;
    std::mt19937       rng(rd());
    std::uniform_int_distribution<size_t> dist(0, max_job_id);

    JobID split_job_id = dist(rng);

    std::set<JobID> groupA;
    std::set<JobID> groupB;

    for (const auto& job_id : parent1.chromosome) {
        if (job_id < split_job_id) {
            groupA.insert(job_id);
        }
        else {
            groupB.insert(job_id);
        }
    }

    Chromosome child1;
    child1.reserve(parent1.chromosome.size());
    std::vector<JobID> reserve1;
    Chromosome         child2;
    child2.reserve(parent1.chromosome.size());
    std::vector<JobID> reserve2;

    for (JobID parent1_job_id : parent1.chromosome) {
        if (groupB.contains(parent1_job_id)) {
            reserve1.push_back(parent1_job_id);
        }
    }
    // reverse reserve1, so we can pop from the back
    std::reverse(reserve1.begin(), reserve1.end());

    for (JobID parent2_job_id : parent2.chromosome) {
        if (groupB.contains(parent2_job_id)) {
            reserve2.push_back(parent2_job_id);
        }
    }
    // reverse reserve2
    std::reverse(reserve2.begin(), reserve2.end());

    for (JobID i = 0; i < parent1.chromosome.size(); ++i) {
        if (groupA.contains(parent1.chromosome[i])) {
            child1[i] = parent1.chromosome[i];
        }
        else {
            child1[i] = reserve2.back();
            reserve2.pop_back();
        }
    }

    for (JobID i = 0; i < parent2.chromosome.size(); ++i) {
        if (groupB.contains(parent2.chromosome[i])) {
            child2[i] = parent2.chromosome[i];
        }
        else {
            child2[i] = reserve1.back();
            reserve1.pop_back();
        }
    }

    return std::make_pair(child1, child2);
}

void GAAlgorithm::mutation(Individual&            individual,
                           const JobShopInstance& instance)
{
    // random select 3 position of the chromosome, and generate all
    // permutations（with swap the value of 3 values）, then select the one with
    // best fitness
    std::random_device                    rd;
    std::mt19937                          rng(rd());
    std::uniform_int_distribution<size_t> dist(
        0, individual.chromosome.size() - 1);

    // random select 3 position of the chromosome
    std::set<size_t> positions_set;
    std::set<JobID>  jobs_set;
    while (positions_set.size() < 3) {
        auto selected_position = dist(rng);
        if (jobs_set.insert(individual.chromosome[selected_position]).second) {
            positions_set.insert(selected_position);
        }
    }
    std::vector<size_t> positions(positions_set.begin(), positions_set.end());
    std::ranges::sort(positions);

    std::vector<Chromosome> permutations;
    // Create a vector with the values at the three positions
    std::vector<JobID> values = {individual.chromosome[positions[0]],
                                 individual.chromosome[positions[1]],
                                 individual.chromosome[positions[2]]};

    std::cout << "mutation positions: \n";
    for (int i = 0; i < 3; ++i) {
        std::cout << "position: " << positions[i] << " value: " << values[i]
                  << std::endl;
    }

    // Sort the values
    std::ranges::sort(values);

    // Generate all permutations
    std::cout << "New chromosome: \n";
    do {
        Chromosome new_chromosome    = individual.chromosome;
        new_chromosome[positions[0]] = values[0];
        new_chromosome[positions[1]] = values[1];
        new_chromosome[positions[2]] = values[2];
        permutations.push_back(new_chromosome);

        for (const auto& gene : new_chromosome) {
            std::cout << gene << " ";
        }
        std::cout << std::endl;

    } while (std::next_permutation(values.begin(), values.end()));

    // Select the one with best fitness
    std::vector<Solution> solutions;
    for (const auto& chromosome : permutations) {
        solutions.push_back(decode(chromosome, instance));
    }

    std::ranges::sort(solutions, [](const auto& lhs, const auto& rhs) {
        return lhs.makespan < rhs.makespan;
    });

    auto best_solution = std::move(solutions[0]);



    std::cout << "The updated makespan: " << best_solution.makespan
              << std::endl;

    individual.fitness    = best_solution.makespan;
    individual.chromosome = best_solution.chromo_;

    std::cout << "The updated chromosome: \n";
    for (const auto& gene : individual.chromosome) {
        std::cout << gene << " ";
    }
    std::cout << std::endl;
}



}   // namespace scheduling
