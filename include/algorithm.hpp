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
        , population_size(population_size){};


    static Individual encode(const Solution& solution);
    static Chromosome encode(const JobShopInstance& instance);
    static Solution   decode(const Chromosome&      chromosome,
                             const JobShopInstance& instance);
    static Chromosome tournament_selection(const Population& population);
    static Chromosome crossover(const Chromosome& parent1,
                                const Chromosome& parent2);
    static void       mutation(Chromosome& chromosome);

    void solve(const JobShopInstance& instance, Solution& global_best,
               std::mutex& gbest_mtx) override
    {
        SolutionConstructor constructor = SolutionConstructor();
        constructor.convert_from_instance(instance);
        auto sol = constructor.schedule();
        sol.print();

        std::cout << "Solution from GA \n";
        std::cout << "num threads: " << num_thread_
                  << " Pop size: " << population_size << std::endl;

        auto individual = encode(sol);
        std::cout << "Individual fitness: " << individual.fitness << std::endl;
        std::cout << "Individual chromosome: ";
        for (const auto& gene : individual.chromosome) {
            std::cout << gene << " ";
        }
        std::cout << std::endl;


        Chromosome chromosome = encode(instance);
        std::cout << "Chromosome from instance: ";
        for (const auto& gene : chromosome) {
            std::cout << gene << " ";
        }
        std::cout << std::endl;
    };

private:
    int                     population_size;
    std::mutex              pbest_mtx;
    std::vector<Individual> pbests_;
};

}   // namespace scheduling