#pragma once

#include "algorithm.hpp"
#include "jobShopInstance.hpp"
#include "solution.hpp"
#include "solutionConstructor.hpp"

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>

namespace scheduling {

class Run
{
public:
    Run(const JobShopInstance& instance, int num_threads, int time_limit)
        : instance_(instance)
        , num_threads_(num_threads)
        , time_limit_(time_limit)
    {
        SolutionConstructor constructor;
        constructor.convert_from_instance(instance);
        gbest_ = constructor.schedule();
        std::cout << "Initial gbest fitness: " << gbest_.makespan << "\n";
    };

    void operator()()
    {
        // global_best_.print();
        add_ga_algorithm(num_threads_);
        for (const auto& algorithm : algorithms_) {
            algorithm->solve(instance_, gbest_, gbest_mtx_);
        }
        std::cout << "Final gbest fitness: " << gbest_.makespan << "\n";
    }

    void allocate_threads()
    {
        int num_thread = num_threads_ / algorithms_.size();
        for (int i = 0; i < algorithms_.size(); i++) {
            algorithm_threads_.push_back(num_thread);
        }
    }

    void add_ga_algorithm(int num_threads)
    {
        auto genatic_algorithm =
            std::make_unique<GAAlgorithm>(num_threads, time_limit_, 100);
        algorithms_.push_back(std::move(genatic_algorithm));
    };

    void print_gbest()
    {
        std::shared_lock<std::shared_mutex> lock(gbest_mtx_);
        gbest_.print();
    };

private:
    JobShopInstance                         instance_;
    Solution                                gbest_;
    std::shared_mutex                       gbest_mtx_;
    std::vector<std::unique_ptr<Algorithm>> algorithms_;
    std::vector<int>                        algorithm_threads_;
    int                                     num_threads_;
    int                                     time_limit_;
};

}   // namespace scheduling