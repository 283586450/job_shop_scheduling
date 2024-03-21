#include <gtest/gtest.h>

#include "jobShopInstance.hpp"

TEST(SolutionConstructorTest, ConvertFromInstance)
{
    // 创建一个JobShopInstance对象
    scheduling::JobShopInstance instance;
    // 填充instance的数据...
    instance.generate_instance(10, 10);

    // 检查jobs_的大小
    EXPECT_EQ(instance.jobs().size(), 10);

    // 检查machines_的大小
    EXPECT_EQ(instance.machines().size(), 10);

    // 检查每个job那steps的大小
    for (const auto& [job_id, job] : instance.jobs()) {
        EXPECT_EQ(job.steps.size(), 10);
    }
}