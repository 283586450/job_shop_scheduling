#pragma once

#include <map>

#include "types.hpp"
namespace scheduling {

struct Step
{
    JobID      job_id;
    StepID     step_id;
    MachineID  machine_id;
    TimePeriod duration;
};

struct Job
{
    JobID                  job_id;
    TimeStamp              due_date;
    std::map<StepID, Step> steps;
};

struct Machine
{
    MachineID machine_id;
};


class JobShopInstance
{
public:
    JobShopInstance()  = default;
    ~JobShopInstance() = default;

    // copy constructor
    JobShopInstance(const JobShopInstance& other) = default;
    // copy assignment operator
    JobShopInstance& operator=(const JobShopInstance& other)
    {
        if (this != &other) {
            jobs_     = other.jobs_;
            machines_ = other.machines_;
        }
        return *this;
    };

    // del move semantics
    JobShopInstance(JobShopInstance&&)            = delete;
    JobShopInstance& operator=(JobShopInstance&&) = delete;

    std::map<JobID, Job>         jobs() const { return jobs_; }
    std::map<MachineID, Machine> machines() const { return machines_; }

    void add_job(JobID job_id, TimeStamp due_date)
    {
        jobs_[job_id] = Job{job_id, due_date};
    }
    void add_machine(MachineID machine_id)
    {
        machines_[machine_id] = Machine{machine_id};
    }
    void add_step(JobID job_id, StepID step_id, MachineID machine_id,
                  TimePeriod duration)
    {
        jobs_[job_id].steps[step_id] =
            Step{job_id, step_id, machine_id, duration};
    }

    std::map<JobID, Job>         jobs() { return jobs_; }
    std::map<MachineID, Machine> machines() { return machines_; }

    void generate_instance(int num_job, int num_machine);
    void print() const;

private:
    std::map<JobID, Job>         jobs_;
    std::map<MachineID, Machine> machines_;
};

}   // namespace scheduling