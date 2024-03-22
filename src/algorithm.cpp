#include "algorithm.hpp"
#include "jobShopInstance.hpp"
#include "solution.hpp"
#include "solutionConstructor.hpp"
#include "types.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <thread>
#include <utility>
#include <vector>


namespace scheduling {


Individual GAAlgorithm::encode(const Solution& solution)
{
    Individual individual;
    individual.fitness = solution.makespan;
    individual.chromosome.reserve(solution.step_tasks.size());

    // retrive the tasks from solution.step_tasks_ and store them in a vector
    std::vector<std::shared_ptr<StepTask>> tasks;
    tasks.reserve(solution.step_tasks.size());
    for (const auto& [task_id, task] : solution.step_tasks) {
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
    Solution solution{};   // solution obj to be returned

    // machine end time map
    std::map<MachineID, TimeStamp> machine_end_times;
    for (const auto& [machine_id, machine] : instance.machines()) {
        machine_end_times[machine_id] = 0;
    }

    // job current step map, job end time map
    std::map<JobID, StepID>    job_curr_step;
    std::map<JobID, TimeStamp> job_end_time;
    for (const auto& [job_id, job] : instance.jobs()) {
        job_curr_step[job_id] = 0;
        job_end_time[job_id]  = 0;
    }

    // for each gene in the chromosome, get the job_id, step_id, machine_id,
    // start end time, and update the machine_end_times, job_end_time,
    // job_curr_step
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

        solution.step_tasks[{job_id, step_id}] = std::make_shared<StepTask>(
            machine_id,
            instance.jobs().at(job_id).steps.at(step_id).duration,
            start_time,
            end_time,
            job_id,
            step_id);

        solution.schedules[machine_id].push_back(
            solution.step_tasks[{job_id, step_id}]);

        solution.makespan = std::max(solution.makespan, end_time);
    }

    solution.chromo = chromosome;

    return solution;
}

Individual GAAlgorithm::tournament_selection(const Population& population)
{
    std::vector<Individual> tournament;
    tournament.reserve(5);

    std::random_device                    random_dev;
    std::mt19937                          rng(random_dev());
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
    JobID              max_job_id = std::ranges::max(parent1.chromosome);
    std::random_device rnd;
    std::mt19937       rng(rnd());
    std::uniform_int_distribution<size_t> dist(0 + 2, max_job_id - 2);

    JobID split_job_id = dist(rng);

    std::set<JobID> group_one;
    std::set<JobID> group_two;

    for (const auto& job_id : parent1.chromosome) {
        if (job_id < split_job_id) {
            group_one.insert(job_id);
        }
        else {
            group_two.insert(job_id);
        }
    }

    Chromosome         child1;
    std::vector<JobID> reserve1;
    child1.resize(parent1.chromosome.size());
    // child1.resize(parent1.chromosome.size());

    Chromosome         child2;
    std::vector<JobID> reserve2;
    // child2.reserve(parent1.chromosome.size());
    child2.resize(parent1.chromosome.size());

    for (JobID parent2_job_id : parent2.chromosome) {
        if (group_two.contains(parent2_job_id)) {
            reserve1.push_back(parent2_job_id);
        }
    }
    // reverse reserve1, so we can pop from the back
    std::reverse(reserve1.begin(), reserve1.end());

    for (JobID parent1_job_id : parent1.chromosome) {
        if (group_one.contains(parent1_job_id)) {
            reserve2.push_back(parent1_job_id);
        }
    }
    // reverse reserve2
    std::reverse(reserve2.begin(), reserve2.end());

    for (JobID i = 0; i < parent1.chromosome.size(); ++i) {
        if (group_one.contains(parent1.chromosome[i])) {
            child1[i] = parent1.chromosome[i];
        }
        else {
            child1[i] = reserve1.back();
            reserve1.pop_back();
        }
    }

    for (JobID i = 0; i < parent2.chromosome.size(); ++i) {
        if (group_two.contains(parent2.chromosome[i])) {
            child2[i] = parent2.chromosome[i];
        }
        else {
            child2[i] = reserve2.back();
            reserve2.pop_back();
        }
    }

    return {std::move(child1), std::move(child2)};
}

void GAAlgorithm::mutation(Individual&            individual,
                           const JobShopInstance& instance)
{
    // random select 3 position of the chromosome, and generate all
    // permutations（with swap the value of 3 values）, then select the one with
    // best fitness
    std::random_device                    random_device;
    std::mt19937                          random_engine(random_device());
    std::uniform_int_distribution<size_t> dist(
        0, individual.chromosome.size() - 1);

    // random select 3 diff position with 3 diff values from the chromosome
    std::vector<size_t> positions;
    std::vector<JobID>  candidate_job_list;
    while (candidate_job_list.size() < 3) {
        auto selected_position = dist(random_engine);
        auto selected_job      = individual.chromosome[selected_position];
        if (std::find(candidate_job_list.begin(),
                      candidate_job_list.end(),
                      selected_job) == candidate_job_list.end()) {
            positions.push_back(selected_position);
            candidate_job_list.push_back(selected_job);
        }
    }

    // Sort the values
    std::ranges::sort(candidate_job_list);

    // Create a vector with the values at the three positions
    std::vector<JobID> values = {individual.chromosome[positions[0]],
                                 individual.chromosome[positions[1]],
                                 individual.chromosome[positions[2]]};

    // Generate all permutations
    std::vector<Chromosome> permutations;
    while (std::next_permutation(candidate_job_list.begin(),
                                 candidate_job_list.end())) {
        Chromosome new_chromosome    = individual.chromosome;
        new_chromosome[positions[0]] = candidate_job_list[0];
        new_chromosome[positions[1]] = candidate_job_list[1];
        new_chromosome[positions[2]] = candidate_job_list[2];
        permutations.push_back(new_chromosome);
    }

    // Select the one with best fitness
    std::vector<Solution> solutions;
    solutions.reserve(permutations.size());
    for (const auto& chromosome : permutations) {
        solutions.push_back(decode(chromosome, instance));
    }

    std::ranges::sort(solutions, [](const auto& lhs, const auto& rhs) {
        return lhs.makespan < rhs.makespan;
    });

    auto best_solution = std::move(solutions[0]);

    // update the individual
    individual.fitness    = best_solution.makespan;
    individual.chromosome = best_solution.chromo;
}

void GAAlgorithm::solve(const JobShopInstance& instance, Solution& global_best,
                        std::shared_mutex& gbest_mtx)
{
    // multi-threading GA algorithm implementation
    // 1. each thread has its own population,
    // 2. update global best solution with mutex when better solution found
    // 3. update local best solution with mutex when better solutions found

    std::cout << "solve from ga algorithm solve func" << '\n';

    const int         NUM_PBESTS = 10;
    std::atomic<bool> stop{false};
    pbests_.reserve(NUM_PBESTS);

    // create initial pbests
    for (int i = 0; i < NUM_PBESTS; i++) {
        pbests_.push_back(encode(instance));
    }

    // single_thread_ga(instance, global_best, gbest_mtx);

    // Run the GA algorithm in multiple threads
    std::vector<std::thread> threads;
    threads.reserve(num_thread_);
    for (int i = 0; i < num_thread_; ++i) {
        threads.emplace_back(
            [this, &instance, &global_best, &gbest_mtx, &stop]() {
                while (!stop.load()) {
                    this->single_thread_ga(
                        instance, global_best, gbest_mtx, stop);
                }
            });
    }

    // Wait for the specified time limit
    std::this_thread::sleep_for(std::chrono::seconds(time_limit_));

    // Notify the GA algorithm to stop
    stop = true;

    // Wait for all threads to finish
    for (auto& thread : threads) {
        if (thread.joinable()) {
            std::cout << "Thread is still running.\n";
            thread.join();
        }
        else {
            std::cout << "Thread has finished.\n";
        }
    }
}

void GAAlgorithm::single_thread_ga(const JobShopInstance& instance,
                                   Solution&              global_best,
                                   std::shared_mutex&     gbest_mtx,
                                   std::atomic<bool>&     stop)
{
    Population population;
    population.reserve(population_size_);
    for (int i = 0; i < population_size_; i++) {
        population.push_back(encode(instance));
    }


    std::random_device rand_dev;
    std::mt19937       rand_engine(rand_dev());


    while (!stop.load()) {

        // sort the population by fitness
        std::ranges::sort(population, [](const auto& lhs, const auto& rhs) {
            return lhs.fitness < rhs.fitness;
        });


        // check and update the global best solution
        if (population[0].fitness <
            get_gbest_fitnesss(global_best, gbest_mtx)) {
            update_gbest(population[0], instance, gbest_mtx, global_best);
            std::cout << "Current Best Fitness: " << population[0].fitness
                      << '\n';
        }

        // check and update the local best solutions
        if (population[0].fitness <
            get_worst_pbest_fitness(pbests_, pbest_mtx_)) {
            update_pbests(population[0], pbest_mtx_, pbests_);
        }


        Population new_gen;
        new_gen.reserve(population_size_);

        // select the best 10 individuals directly into the new generation
        for (int i = 0; i < 10; i++) {
            new_gen.push_back(population[i]);
        }

        // remove the worst 10 individuals from population
        for (int i = 0; i < 30; i++) {
            population.pop_back();
        }

        // copy the pbests to population, for crossover and mutation
        for (const auto& pbest : pbests_) {
            population.push_back(pbest);
        }

        while (new_gen.size() < population_size_) {

            // select two parents using tournament selection
            std::uniform_real_distribution<float> proba_dist(0, 1);

            // mutation with 30% probability
            if (proba_dist(rand_engine) < 0.3) {
                auto parent1 = tournament_selection(population);
                mutation(parent1, instance);
                new_gen.push_back(parent1);
                continue;
            }

            // crossover with 70% probability
            if (proba_dist(rand_engine) < 0.7) {
                auto       parent1 = tournament_selection(population);
                auto       parent2 = tournament_selection(population);
                Individual child1{};
                Individual child2{};
                // crossover the parents
                auto [chromo1, chromo2] = crossover(parent1, parent2);
                child1.chromosome       = chromo1;
                child2.chromosome       = chromo2;

                auto sol1      = decode(chromo1, instance);
                auto sol2      = decode(chromo2, instance);
                child1.fitness = sol1.makespan;
                child2.fitness = sol2.makespan;

                new_gen.push_back(child1);
                new_gen.push_back(child2);
                continue;
            }
        }
        population = std::move(new_gen);
    }
}

Fitness GAAlgorithm::get_gbest_fitnesss(const Solution&    global_best,
                                        std::shared_mutex& gbest_mtx)
{
    std::shared_lock<std::shared_mutex> lock(gbest_mtx);
    return global_best.makespan;
}


Fitness GAAlgorithm::get_worst_pbest_fitness(
    const std::vector<Individual>& pbests, std::shared_mutex& pbest_mtx)
{
    std::shared_lock<std::shared_mutex> lock(pbest_mtx);
    if (pbests.empty()) {
        return std::numeric_limits<Fitness>::max();
    }
    return std::min_element(pbests.begin(),
                            pbests.end(),
                            [](const auto& lhs, const auto& rhs) {
                                return lhs.fitness < rhs.fitness;
                            })
        ->fitness;
}

void GAAlgorithm::update_gbest(const Individual&      individual,
                               const JobShopInstance& instance,
                               std::shared_mutex&     gbest_mtx,
                               Solution&              global_best)
{
    std::unique_lock<std::shared_mutex> lock(gbest_mtx);
    if (individual.fitness < global_best.makespan) {
        global_best = decode(individual.chromosome, instance);
    }
}

void GAAlgorithm::update_pbests(const Individual&        individual,
                                std::shared_mutex&       pbest_mtx,
                                std::vector<Individual>& pbests)
{
    std::unique_lock<std::shared_mutex> lock(pbest_mtx);
    if (pbests.size() < 10) {
        pbests.push_back(individual);
    }
    else {
        auto worst_pbest = std::max_element(
            pbests.begin(), pbests.end(), [](const auto& lhs, const auto& rhs) {
                return lhs.fitness < rhs.fitness;
            });
        if (individual.fitness < worst_pbest->fitness) {
            *worst_pbest = individual;
        }
    }
}


}   // namespace scheduling
