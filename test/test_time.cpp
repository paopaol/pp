#include <gtest/gtest.h>
#include <time/_time.h>

using namespace pp;

namespace {
class test_time_time : public ::testing::Test {
};
}  // namespace

TEST_F(test_time_time, test_time_functions)
{
    _time::time now;

    auto then = now.add(_time::Hour * 100);
    EXPECT_EQ(then.equal(now), true);
    EXPECT_EQ(then.sub(now), 0);

    then.add(_time::Second * 120);
    EXPECT_EQ(then.after(now), true);
    EXPECT_EQ(now.before(then), true);
    std::cout << then.format() << std::endl;

    int64_t unix = then.unix();

    struct tm* tm = localtime(&unix);

    EXPECT_EQ(then.year(), tm->tm_year + 1900);
    EXPECT_EQ(then.month().number(), tm->tm_mon + 1);
    EXPECT_EQ(then.mday(), tm->tm_mday);
    EXPECT_EQ(then.hour(), tm->tm_hour);
    EXPECT_EQ(then.minute(), tm->tm_min);
    EXPECT_EQ(then.second(), tm->tm_sec);
    // EXPECT_EQ(then.Year(), tm->tm_year);
}
