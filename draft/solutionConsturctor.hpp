#pragma once

#include "jobShopInstance.hpp"
#include "solution.hpp"
#include "types.hpp"
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <queue>
#include <vector>



namespace scheduling {



class SolutionConstructor
{

    struct StepScheduler
    {
        MachineID  machine_id;
        JobID      job_id;
        StepID     step_id;
        TimePeriod duration;
        bool       ready      = false;
        TimeStamp  start_time = std::numeric_limits<int>::max();
        TimeStamp  end_time   = std::numeric_limits<int>::max();
        TimeStamp  ready_time = std::numeric_limits<int>::max();
    };

    struct JobScheduler
    {
        JobID                                            job_id;
        TimeStamp                                        due_date;
        std::map<StepID, std::shared_ptr<StepScheduler>> steps;
    };

    struct MachineScheduler
    {
        MachineID                                   machine_id;
        TimeStamp                                   curr_time = 0;
        TimeStamp                                   plan_time = 0;
        std::vector<std::shared_ptr<StepScheduler>> ready_list;
    };

    struct CompareMachineScheduler
    {
        bool operator()(const std::shared_ptr<MachineScheduler>& lhs,
                        const std::shared_ptr<MachineScheduler>& rhs) const
        {
            // used for the priority queue of MachineScheduler,
            // because priority queue is a max heap, so we need < operator here
            // if (lhs->plan_time != rhs->plan_time) {
            //     return lhs->plan_time > rhs->plan_time;
            // }
            return lhs->curr_time > rhs->curr_time;
        }
    };


    struct CompareStepScheduler
    {
        bool operator()(const std::shared_ptr<StepScheduler>& lhs,
                        const std::shared_ptr<StepScheduler>& rhs) const
        {
            if (lhs->ready_time != rhs->ready_time) {
                return lhs->ready_time < rhs->ready_time;
            }
            return lhs->duration < rhs->duration;
        }
    };

    int                            random_select_index(size_t size) const;
    std::shared_ptr<StepScheduler> select_next_step(
        const std::shared_ptr<MachineScheduler>& machine);

public:
    void add_job_scheduler(const Job& job);
    void add_machine_scheduler(const Machine& machine);
    void add_step_scheduler(const Step& step);
    void convert_from_instance(const JobShopInstance& instance);

    using MachineCompareFunc =
        std::function<bool(const std::shared_ptr<MachineScheduler>&,
                           const std::shared_ptr<MachineScheduler>&)>;
    using MachinePriorityQueue =
        std::priority_queue<std::shared_ptr<MachineScheduler>,
                            std::vector<std::shared_ptr<MachineScheduler>>,
                            MachineCompareFunc>;
    [[nodiscard]] MachinePriorityQueue create_machine_priority_queue(
        const MachineCompareFunc& compare) const
    {
        MachinePriorityQueue queue(compare);
        for (const auto& machine : machines) {
            queue.push(std::make_shared<MachineScheduler>(machine));
        }
        return queue;
    };

    Solution schedule();
    void     print() const;

    std::map<JobID, JobScheduler>                    get_jobs() { return jobs; }
    std::map<TaskID, std::shared_ptr<StepScheduler>> get_waiting_list()
    {
        return waiting_list;
    }
    std::vector<MachineScheduler> get_machines() { return machines; }


private:
    float                                            alpha_ = 0.3;
    std::map<JobID, JobScheduler>                    jobs;
    std::map<TaskID, std::shared_ptr<StepScheduler>> waiting_list;
    std::vector<MachineScheduler>                    machines;
    // store the results
    std::map<TaskID, std::shared_ptr<StepTask>>     step_tasks_;
    std::map<TaskID, std::shared_ptr<PMTask>>       pm_tasks_;
    std::map<int, std::vector<std::weak_ptr<Task>>> schedules_;
};


}   // namespace scheduling
