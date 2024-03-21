#include "solutionConsturctor.hpp"
#include "jobShopInstance.hpp"
#include "solution.hpp"
#include "types.hpp"
#include <algorithm>
#include <cstddef>
#include <format>
#include <iostream>
#include <memory>
#include <random>
#include <utility>

namespace scheduling {
using std::sort;


void SolutionConstructor::add_job_scheduler(const Job& job)
{
    JobScheduler job_scheduler{job.job_id, job.due_date};
    jobs[job_scheduler.job_id] = job_scheduler;
}

void SolutionConstructor::add_machine_scheduler(const Machine& machine)
{
    MachineScheduler machine_scheduler{machine.machine_id};
    machines.push_back(machine_scheduler);
}

void SolutionConstructor::add_step_scheduler(const Step& step)
{
    std::shared_ptr<StepScheduler> step_scheduler_ptr =
        std::make_shared<StepScheduler>(StepScheduler{
            step.machine_id, step.job_id, step.step_id, step.duration});

    TaskID task_id = std::make_pair(step.job_id, step.step_id);
    if (step.step_id == 0) {
        step_scheduler_ptr->ready      = true;
        step_scheduler_ptr->ready_time = 0;
        machines[step.machine_id].ready_list.push_back(step_scheduler_ptr);
    }
    else {
        waiting_list[task_id] = step_scheduler_ptr;
    }

    // add the shared_ptr to jobs
    jobs[step.job_id].steps[step.step_id] = step_scheduler_ptr;
}



void SolutionConstructor::convert_from_instance(const JobShopInstance& instance)
{
    // convert and add machine obj
    for (const auto& [machine_id, machine] : instance.machines()) {
        add_machine_scheduler(machine);
    }

    // convert and add job obj
    for (const auto& [job_id, job] : instance.jobs()) {
        add_job_scheduler(job);

        // convert and add step obj
        for (const auto& [step_id, step] : job.steps) {
            add_step_scheduler(step);
        }
    }
}



int SolutionConstructor::random_select_index(size_t size) const
{
    if (size == 0) {
        throw std::invalid_argument("size should not be 0");
    }

    int ub_limit = static_cast<int>(size * alpha_);

    std::random_device                 rnd_dev;
    std::mt19937                       rnd_engine(rnd_dev());
    std::uniform_int_distribution<int> index_distro(0, ub_limit);
    return index_distro(rnd_engine);
}

std::shared_ptr<SolutionConstructor::StepScheduler>
SolutionConstructor::select_next_step(
    const std::shared_ptr<SolutionConstructor::MachineScheduler>& machine)
{
    // 1. sort the ready list
    std::ranges::sort(machine->ready_list, CompareStepScheduler());

    // 2. random select the index of the ready list
    int select_index = random_select_index(machine->ready_list.size());

    auto selected_step_ptr = machine->ready_list[select_index];

    // 3. remove the selected step from the ready list
    machine->ready_list.erase(machine->ready_list.begin() + select_index);

    return selected_step_ptr;
}


Solution SolutionConstructor::schedule()
{
    // 1. create a machine priority queue
    auto machine_queue =
        create_machine_priority_queue(CompareMachineScheduler());

    int iter = 0;
    while (!machine_queue.empty()) {
        iter += 1;
        if (iter >= 5000) {
            break;
        }

        auto selected_machine = machine_queue.top();
        machine_queue.pop();   // del from the queue

        auto msg =
            std::format("The ready list size of machine {} is {}, plan_time = "
                        "{}, curr_time = {}, waiting list size: {} \n",
                        selected_machine->machine_id,
                        selected_machine->ready_list.size(),
                        selected_machine->plan_time,
                        selected_machine->curr_time,
                        waiting_list.size());
        // std::cout << msg;

        if (selected_machine->ready_list.empty()) {
            if (waiting_list.empty()) {
                continue;
            }
            else {
                selected_machine->plan_time += 3;
                machine_queue.push(selected_machine);
                continue;
            }
        }

        // 2. sort the ready list and randome select next step, remove the
        // selected step from ready_list
        auto selected_step_ptr = select_next_step(selected_machine);

        // 3. update the current step and machine
        TimeStamp task_start_time = std::max(selected_machine->curr_time,
                                             selected_step_ptr->ready_time);
        TimeStamp task_end_time = task_start_time + selected_step_ptr->duration;
        selected_step_ptr->start_time = task_start_time;
        selected_step_ptr->end_time   = task_end_time;
        selected_machine->curr_time   = task_end_time;
        selected_machine->plan_time =
            std::max(task_end_time, selected_machine->plan_time);

        // add the task into step_tasks_ and schedules_
        TaskID curr_task_id = std::make_pair(selected_step_ptr->job_id,
                                             selected_step_ptr->step_id);
        // create a step task shared ptr
        auto select_task_ptr =
            std::make_shared<StepTask>(StepTask{{selected_step_ptr->machine_id,
                                                 selected_step_ptr->duration,
                                                 selected_step_ptr->start_time,
                                                 selected_step_ptr->end_time},
                                                selected_step_ptr->job_id,
                                                selected_step_ptr->step_id,
                                                TaskType::STEP});
        step_tasks_[curr_task_id] = select_task_ptr;
        schedules_[selected_machine->machine_id].push_back(
            std::weak_ptr<StepTask>(select_task_ptr));

        // 4. update the time of next step and add it to ready_list
        auto   job_id       = selected_step_ptr->job_id;
        auto   next_step_id = selected_step_ptr->step_id + 1;
        TaskID next_task_id = std::make_pair(job_id, next_step_id);

        // check if the next step still in waiting list
        if (waiting_list.contains(next_task_id)) {
            auto next_step_ptr        = waiting_list[next_task_id];
            next_step_ptr->ready      = true;
            next_step_ptr->ready_time = task_end_time;
            auto next_machine_id      = next_step_ptr->machine_id;
            machines[next_machine_id].ready_list.push_back(next_step_ptr);
            waiting_list.erase(next_task_id);
        }

        // if the machine's ready list and waiting list are empty,
        // then do not push the machine back to the queue
        if (selected_machine->ready_list.empty() && waiting_list.empty()) {
            continue;
        }

        machine_queue.push(selected_machine);
    }
    return Solution{step_tasks_, pm_tasks_, schedules_};
}

void SolutionConstructor::print() const
{

    for (const auto& [task_id, step] : step_tasks_) {
        std::cout << "Task " << task_id.first << " " << task_id.second
                  << " start time: " << step->start_time
                  << " end time: " << step->end_time << '\n';
    }
    for (const auto& [machine_id, schedule] : schedules_) {
        std::cout << "Machine " << machine_id << " schedule: ";
        for (const auto& task : schedule) {
            if (auto ptr = task.lock()) {
                std::cout << "[" << ptr->start_time << " " << ptr->end_time
                          << "] ";
            }
        }
        std::cout << '\n';
    }
}

}   // namespace scheduling