#pragma once

#include <memory>
#include <shared_mutex>
#include <vector>

#include "algorithm.hpp"
#include "jobShopInstance.hpp"

namespace scheduling {

Solution generate_init_sol(const JobShopInstance& instance, float alpha);

class JobShopScheduling
{
public:
    JobShopScheduling(const JobShopInstance& instance, int total_threads,
                      int time_limit)
        : instance_(instance)
        , total_threads_(total_threads)
        , time_limit_(time_limit){};

    void add_algorithm(Algorithm algorithm, int num_thread, int time_limit);
    void
    solve();   // pass instance, gbest_mtx_, gbest_solution_ to each algorithm
    Solution gbest_solution() const;

private:
    JobShopInstance                         instance_;
    int                                     total_threads_;
    int                                     time_limit_;
    std::vector<std::unique_ptr<Algorithm>> algorithms_;
    std::shared_mutex                       gbest_mtx_;
    Solution                                gbest_solution_;
};

}   // namespace scheduling