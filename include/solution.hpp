#pragma once

#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <vector>

#include "disjunctiveGraph.hpp"
#include "jobShopInstance.hpp"
#include "types.hpp"


namespace scheduling {


struct Task
{
    MachineID  machine_id;
    TimePeriod duration;
    TimeStamp  start_time = 0;
    TimeStamp  end_time   = std::numeric_limits<TimeStamp>::max();

    Task(MachineID machine_id, TimePeriod duration, TimeStamp start_time,
         TimeStamp end_time)
        : machine_id(machine_id)
        , duration(duration)
        , start_time(start_time)
        , end_time(end_time)
    {}

    virtual ~Task() = default;
};

struct StepTask : public Task
{
    JobID    job_id;
    StepID   step_id;
    TaskType task_type = TaskType::STEP;

    StepTask(MachineID machine_id, TimePeriod duration, TimeStamp start_time,
             TimeStamp end_time, JobID job_id, StepID step_id)
        : Task{machine_id, duration, start_time, end_time}
        , job_id(job_id)
        , step_id(step_id)
    {}
};

struct PMTask : public Task
{
    int      pm_id;
    TaskType task_type = TaskType::PM;

    // to be added later in the project
};

struct Solution
{
    std::map<TaskID, std::shared_ptr<StepTask>>     step_tasks_;
    std::map<TaskID, std::shared_ptr<PMTask>>       pm_tasks_;
    std::map<int, std::vector<std::weak_ptr<Task>>> schedules_;
    std::unique_ptr<DisjunctiveGraph>               graph_  = nullptr;
    std::unique_ptr<Chromosome>                     chromo_ = nullptr;
    unsigned int                                    makespan;

    void update_makespan()
    {
        makespan = 0;
        for (const auto& [machine_id, tasks] : schedules_) {
            for (const auto& task : tasks) {
                auto locked_task = task.lock();
                if (auto step_task =
                        std::dynamic_pointer_cast<StepTask>(locked_task)) {
                    if (step_task->end_time > makespan) {
                        makespan = step_task->end_time;
                    }
                }
                else if (auto pm_task =
                             std::dynamic_pointer_cast<PMTask>(locked_task)) {
                    if (pm_task->end_time > makespan) {
                        makespan = pm_task->end_time;
                    }
                }
            }
        }
    };

    void print()
    {
        std::cout << "Solution: \n";
        for (const auto& [machine_id, tasks] : schedules_) {
            std::cout << "Machine " << machine_id << ": ";
            for (const auto& task : tasks) {
                auto locked_task = task.lock();
                if (auto step_task =
                        std::dynamic_pointer_cast<StepTask>(locked_task)) {
                    std::cout << "Job :" << step_task->job_id
                              << " Step: " << step_task->step_id
                              << " Start: " << step_task->start_time << " "
                              << "End: " << step_task->end_time << " \n ";
                }
                else if (auto pm_task =
                             std::dynamic_pointer_cast<PMTask>(locked_task)) {
                    std::cout << "PM :" << pm_task->pm_id
                              << " Start: " << pm_task->start_time << " "
                              << "End: " << pm_task->end_time << " \n ";
                }
            }
            std::cout << "\n";
        }

        std::cout << "Makespan: " << makespan << "\n";
    }
};

}   // namespace scheduling