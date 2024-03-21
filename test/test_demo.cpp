#include <gtest/gtest.h>

int demo(int a, int b)
{
    return a + b;
}

TEST(DemoTest, AdditionTest)
{
    // Test case 1
    EXPECT_EQ(demo(2, 3), 5);

    // Test case 2
    EXPECT_EQ(demo(-5, 10), 5);

    // Test case 3
    EXPECT_EQ(demo(0, 0), 0);
}
