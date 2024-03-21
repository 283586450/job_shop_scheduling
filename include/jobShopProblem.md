#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <utility>
#include <vector>


using TaskID = std::pair<int, int>;


class Task;
class DisjunctiveGraph;
class Chromosome;

class JobShopInstance
{
    // no move semantics, only copy semantics
};

class Solution
{
    // no move semantics, only copy semantics
public:
    std::map<TaskID, Task> S0;   // <job,step>:task
    std::map<int, TaskID>  S1;   // <machine>: vector<task>
    DisjunctiveGraph       graph;
    Chromosome             chromo;

    void to_disjunctive_graph();
    void to_chromosome();
    void print();
};




class Algorithm
{
protected:
    int                   num_thread_;
    int                   time_limit_;
    JobShopInstance       instance;
    Solution              solution;
    std::mutex            pbests_mtx;
    std::vector<Solution> pbests;

public:
    virtual Solution solve(
        const JobShopInstance& instance) = 0;   // Pure virtual function
};


class LocalSearch : public Algorithm
{

public:
    Solution solve(const JobShopInstance& instance) override;
};
class LNS : public Algorithm
{
public:
    Solution solve(const JobShopInstance& instance) override;
};
class TabuSearch : public Algorithm
{
public:
    Solution solve(const JobShopInstance& instance) override;
};
class Sat : public Algorithm
{
public:
    Solution solve(const JobShopInstance& instance) override;
};

class JobShopScheduling
{
private:
    int                                     total_threads;
    int                                     time_limit;
    JobShopInstance                         instance;
    std::vector<std::unique_ptr<Algorithm>> algorithms;
    std::vector<int>                        algorithm_threads_ratio;
    std::shared_mutex                       gbest_mtx;
    Solution                                gbest_solution;

public:
    JobShopScheduling(const JobShopInstance& instance);
    void addAlgorithm(std::unique_ptr<Algorithm> algorithm);
    void solve()
    {
        for (auto& algorithm : algorithms) {
            Solution solution = algorithm->solve(
                instance);   // Pass a copy of the instance data
            // Update bestSolution if necessary
        }
    };
    Solution getGbestSolution() const;
};
