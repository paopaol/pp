#include <gtest/gtest.h>
#include <io/io_event_loop.h>
#include <net/net_tcp_connector.h>

using namespace pp;

const static int kBadPort = 808;
const static int kPort    = 80;

const static char* kAddr    = "220.181.112.244";
const static char* kBadAddr = "xxxx.xxxx.xxx.111";

TEST(net_tcp_connector, connect_success)
{
    errors::error_code err;
    io::event_loop     loop;
    // baidu
    auto connector =
        std::make_shared<net::tcp_connector>(&loop, net::addr(kAddr, kPort));

    connector->set_new_conn_handler(
        [&](int fd, const errors::error_code& error) {
            EXPECT_EQ(error.value(), 0);
            loop.quit();
        });
    connector->start_connect(0, err);
    EXPECT_EQ(err.value(), 0);
    loop.exec();
}

TEST(net_tcp_connector, connect_timeout)
{
    errors::error_code err;
    io::event_loop     loop;
    // baidu, 808 is bad port
    auto connector =
        std::make_shared<net::tcp_connector>(&loop, net::addr(kAddr, kBadPort));

    connector->set_new_conn_handler(
        [&](int fd, const errors::error_code& error) {
            EXPECT_NE(error.value(), 0);
            loop.quit();
        });
    connector->start_connect(_time::Second * 5, err);
    EXPECT_EQ(err.value(), 0);
    loop.exec();
}

TEST(net_tcp_connector, connect_failed)
{
    errors::error_code err;
    io::event_loop     loop;
    // baidu, 808 is bad port
    auto connector = std::make_shared<net::tcp_connector>(
        &loop, net::addr(kBadAddr, kBadPort));

    connector->set_new_conn_handler(
        [&](int fd, const errors::error_code& error) {
            EXPECT_NE(error.value(), 0);
            std::string e = error.message();
            loop.quit();
        });
    connector->start_connect(0, err);
    EXPECT_EQ(err.value(), 0);
    loop.exec();
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
