#pragma once

#include "jobShopInstance.hpp"
#include "solution.hpp"
#include "types.hpp"
#include <algorithm>
#include <cstddef>
#include <map>
#include <memory>
#include <random>
#include <vector>


namespace scheduling {

class SolutionConstructor
{
    struct MachineScheduler
    {
        MachineID machine_id;
        TimeStamp current_time;
        TimeStamp plan_time;
    };

    struct StepScheduler
    {
        JobID      job_id;
        StepID     step_id;
        MachineID  machine_id;
        TimePeriod duration;
        bool       ready;
        TimeStamp  ready_time;
        TimeStamp  start_time;
        TimeStamp  end_time;
    };

public:
    void convert_from_instance(const JobShopInstance& instance)
    {
        // add steps to waiting list
        for (const auto& [job_id, job_obj] : instance.jobs()) {
            for (const auto& [step_id, step_obj] : job_obj.steps) {
                if (step_id == 0) {
                    auto step_scheduler = StepScheduler{job_id,
                                                        step_id,
                                                        step_obj.machine_id,
                                                        step_obj.duration,
                                                        true,
                                                        0,
                                                        0,
                                                        0};
                    ready_list[step_obj.machine_id].push_back(step_scheduler);
                }
                else {
                    waiting_list[{job_id, step_id}] =
                        StepScheduler{job_id,
                                      step_id,
                                      step_obj.machine_id,
                                      step_obj.duration,
                                      false,
                                      0,
                                      0,
                                      0};
                }
            }
        }

        // add machines
        for (const auto& [machine_id, machine_obj] : instance.machines()) {
            machines.push_back({machine_id, 0, 0});
        }
    };

    Solution schedule()
    {
        Solution solution;

        size_t total_steps = waiting_list.size();
        for (const auto& [machine_id, machine_ready_list] : ready_list) {
            total_steps += machine_ready_list.size();
        }

        while (total_steps > 0) {
            // 1. sort the machines by plan time with ranges
            std::ranges::sort(machines, [](const auto& lhs, const auto& rhs) {
                return lhs.plan_time < rhs.plan_time;
            });


            auto& selected_machine_ref = machines[0];
            auto  selected_machine_id  = selected_machine_ref.machine_id;

            // if the ready list of selected machine is empty, then increase the
            // plan time and continue
            if (ready_list[selected_machine_id].empty()) {
                selected_machine_ref.plan_time += 3;
                continue;
            }

            // 2. sort the ready_list of the machine by ready time and duration
            // with ranges
            std::ranges::sort(ready_list[selected_machine_id],
                              [](const auto& lhs, const auto& rhs) {
                                  //   return lhs.ready_time < rhs.ready_time;
                                  if (lhs.ready_time == rhs.ready_time) {
                                      return lhs.duration < rhs.duration;
                                  }
                                  else {
                                      return lhs.ready_time < rhs.ready_time;
                                  }
                              });

            // use GRASP algorithm to random select the step
            size_t num_ready_steps = ready_list[selected_machine_id].size();
            size_t ub_selected_step_index =
                std::max(1UL, static_cast<size_t>(num_ready_steps * alpha));

            std::random_device                    rnd_dev;
            std::mt19937                          rnd_engine(rnd_dev());
            std::uniform_int_distribution<size_t> step_index_distro(
                0, ub_selected_step_index - 1);
            size_t selected_step_index = step_index_distro(rnd_engine);

            // 3. get the first step obj from the sorted ready_list
            auto selected_step_obj =
                ready_list[selected_machine_id][selected_step_index];
            // del the selected step from the ready_list
            ready_list[selected_machine_id].erase(
                ready_list[selected_machine_id].begin() + selected_step_index);

            TimeStamp start_time = std::max(selected_machine_ref.current_time,
                                            selected_step_obj.ready_time);
            TimeStamp end_time   = start_time + selected_step_obj.duration;

            // 4. update the selected machine's current time and plan time
            selected_machine_ref.current_time = end_time;
            selected_machine_ref.plan_time =
                std::max(selected_machine_ref.plan_time,
                         selected_machine_ref.current_time);

            // 5. update the selected step's start time and end time
            selected_step_obj.start_time = start_time;
            selected_step_obj.end_time   = end_time;


            // create a steptask and add it to solution.step_tasks and
            // schedules_
            auto step_task = std::make_shared<StepTask>(
                StepTask{selected_step_obj.machine_id,
                         selected_step_obj.duration,
                         selected_step_obj.start_time,
                         selected_step_obj.end_time,
                         selected_step_obj.job_id,
                         selected_step_obj.step_id});

            solution.step_tasks_[{selected_step_obj.job_id,
                                  selected_step_obj.step_id}] = step_task;
            solution.schedules_[selected_step_obj.machine_id].push_back(
                std::weak_ptr<Task>(step_task));

            total_steps -= 1;   // one step is scheduled

            // 7. check if the next step is exist in waiting list
            // if yes, then add it to the ready list
            auto next_step_id = selected_step_obj.step_id + 1;
            auto next_job_id  = selected_step_obj.job_id;
            auto it           = waiting_list.find({next_job_id, next_step_id});
            if (it != waiting_list.end()) {
                auto next_step_obj  = it->second;
                next_step_obj.ready = true;
                next_step_obj.ready_time =
                    selected_step_obj.end_time;   // update the ready time
                ready_list[next_step_obj.machine_id].push_back(next_step_obj);
                waiting_list.erase({next_job_id, next_step_id});
            }
        }

        solution.update_makespan();
        
        return solution;
    };


private:
    float                                           alpha = 0.3F;
    std::vector<MachineScheduler>                   machines;
    std::map<MachineID, std::vector<StepScheduler>> ready_list;
    std::map<TaskID, StepScheduler>                 waiting_list;
};

}   // namespace scheduling