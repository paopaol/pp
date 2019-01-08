#include <gtest/gtest.h>
#include <io/io_event_loop.h>
#include <sync/sync_chan.h>
#include <system/sys_thread.h>
#include <thread>

using namespace pp;

namespace {
class test_io_event_loop : public ::testing::Test {
protected:
    io::event_loop loop_;
};
}  // namespace

TEST_F(test_io_event_loop, test_in_created_thread)
{
    EXPECT_EQ(loop_.in_created_thread(), true);
}

TEST_F(test_io_event_loop, test_run_in_loop_construct_thread)
{
    loop_.run_in_loop([&]() { EXPECT_EQ(loop_.in_created_thread(), true); });
}

TEST_F(test_io_event_loop, test_run_in_loop_other_thread)
{
    sync::Chan<bool> chan;

    std::thread other_thread([&]() {
        // 3 seconds after, run start run this thread function
        bool notify;
        chan.read(notify);
        loop_.run_in_loop(
            [&]() { EXPECT_EQ(loop_.in_created_thread(), true); });
    });
    std::thread other_thread2([&]() {
        other_thread.join();
        loop_.quit();
    });
    other_thread2.detach();
    _time::new_timer(_time::timer::oneshot, _time::Second * 3, [&]() {
        // emit start run thread signal
        chan.write(true);
    });
    loop_.exec();
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
