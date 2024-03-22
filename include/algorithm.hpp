#pragma once

#include <iostream>
#include <mutex>
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
                       std::mutex& gbest_mtx) = 0;

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

    void solve(const JobShopInstance& instance, Solution& global_best,
               std::mutex& gbest_mtx) override
    {
        SolutionConstructor constructor = SolutionConstructor();
        constructor.convert_from_instance(instance);
        auto sol = constructor.schedule();
        sol.print();

        std::cout << "Solution from GA \n";
        std::cout << "num threads: " << num_thread_
                  << " Pop size: " << population_size_ << '\n';

        auto individual = encode(sol);
        std::cout << "Individual fitness: " << individual.fitness << '\n';
        std::cout << "Individual chromosome: ";
        for (const auto& gene : individual.chromosome) {
            std::cout << gene << " ";
        }
        std::cout << '\n';

        auto new_indi = encode(instance);
        std::cout << "Chromosome from instance: ";
        for (const auto& gene : new_indi.chromosome) {
            std::cout << gene << " ";
        }
        std::cout << '\n';

        mutation(individual, instance);
    };

private:
    int                     population_size_;
    std::mutex              pbest_mtx_;
    std::vector<Individual> pbests_;
};

}   // namespace scheduling