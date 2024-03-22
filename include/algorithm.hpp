#pragma once

#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <vector>

#include "jobShopInstance.hpp"
#include "solution.hpp"
#include "solutionConstructor.hpp"
#include "types.hpp"

namespace scheduling {

class Algorithm
{
protected:
    int num_thread_{};
    int time_limit_{};

public:
    Algorithm()                            = default;
    Algorithm(const Algorithm&)            = delete;
    Algorithm& operator=(const Algorithm&) = delete;
    virtual ~Algorithm()                   = default;

    Algorithm(int num_thread, int time_limit)
        : num_thread_(num_thread)
        , time_limit_(time_limit)
    {}

    virtual void solve(const JobShopInstance& instance, Solution& global_best,
                       std::shared_mutex& gbest_mtx) = 0;

    // del move semantics
    Algorithm(Algorithm&&)            = delete;
    Algorithm& operator=(Algorithm&&) = delete;
};

class GAAlgorithm : public Algorithm
{

public:
    GAAlgorithm(int num_threads, int time_limit, int population_size = 100)
        : Algorithm(num_threads, time_limit)
        , population_size_(population_size){};

    static Individual encode(const Solution& solution);
    static Individual encode(const JobShopInstance& instance);
    static Solution   decode(const Chromosome&      chromosome,
                             const JobShopInstance& instance);
    static Individual tournament_selection(const Population& population);
    static std::pair<Chromosome, Chromosome> crossover(
        const Individual& parent1, const Individual& parent2);
    static void mutation(Individual&            individual,
                         const JobShopInstance& instance);
    static void print_individual(const Individual& individual)
    {
        std::cout << "\n";
        std::cout << "GAAlgorithm Individule: \n";
        std::cout << "Fitness: " << individual.fitness << '\n';
        std::cout << "Chromosome: ";
        for (const auto& gene : individual.chromosome) {
            std::cout << gene << " ";
        }
        std::cout << '\n';
    };

    void solve(const JobShopInstance& instance, Solution& global_best,
               std::shared_mutex& gbest_mtx) override;

private:
    static Fitness get_gbest_fitnesss(const Solution&    global_best,
                                      std::shared_mutex& gbest_mtx);
    static Fitness get_worst_pbest_fitness(
        const std::vector<Individual>& pbests, std::shared_mutex& pbest_mtx);

    static void update_gbest(const Individual&      individual,
                             const JobShopInstance& instance,
                             std::shared_mutex&     gbest_mtx,
                             Solution&              global_best);
    static void update_pbests(const Individual&        individual,
                              std::shared_mutex&       pbest_mtx,
                              std::vector<Individual>& pbests);
    void        single_thread_ga(const JobShopInstance& instance,
                                 Solution& global_best, std::shared_mutex& gbest_mtx,
                                 std::atomic<bool>& stop);

private:
    int                     population_size_;
    std::shared_mutex       pbest_mtx_;
    std::vector<Individual> pbests_;
};

}   // namespace scheduling