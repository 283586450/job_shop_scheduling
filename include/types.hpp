#pragma once

#include <string>
#include <utility>
#include <vector>

namespace scheduling {

using JobID      = unsigned int;
using StepID     = unsigned int;
using MachineID  = unsigned int;
using TimeStamp  = unsigned int;
using TimePeriod = unsigned int;
using TaskID     = std::pair<JobID, StepID>;
using Chromosome = std::vector<unsigned int>;
using Population = std::vector<Chromosome>;

enum class TaskType
{
    STEP,
    PM
};

inline std::string TaskTypeToString(TaskType taskType)
{
    switch (taskType) {
    case TaskType::STEP: return "STEP";
    case TaskType::PM: return "PM";
    default: return "Unknown TaskType";
    }
};

struct Individual
{
    Chromosome chromosome;
    int        fitness;
};



}   // namespace scheduling