#include "jobShopScheduling.hpp"
#include "solution.hpp"
#include "types.hpp"
#include <functional>
#include <queue>

namespace scheduling {

// Solution generate_init_sol(const JobShopInstance& instance, float alpha)
// {
//     // init and add machine into machine_schedule
//     std::map<int, std::vector<TaskID>> machine_schedule;
//     for (const auto& machine : instance.machines()) {
//         machine_schedule[machine.first] = std::vector<TaskID>();
//     }

//     // create a priority queue, and add each MachineInfo into it
//     auto machine_compare = [](MachineInfo lhs, MachineInfo rhs) {
//         return lhs.curr_time > rhs.curr_time;
//     };
//     std::priority_queue<MachineInfo,
//                         std::vector<MachineInfo>,
//                         std::function<bool(MachineInfo, MachineInfo)>>
//         machine_queue(machine_compare);
//     for (const auto& machine : instance.machines()) {
//         machine_queue.push(MachineInfo{machine.first, 0, 0});
//     }

//     // create a ready_task_list for each machine, and add each TaskInfo into it
// }

}   // namespace scheduling