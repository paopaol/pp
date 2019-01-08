#include <gtest/gtest.h>
#include <io/io_event_loop.h>
#include <net/net_tcp_connector.h>

using namespace pp;


TEST(net_tcp_connector, connect_success)
{
    errors::error_code err;
    io::event_loop     loop;
    // baidu
    auto connector = std::make_shared<net::tcp_connector>(
        &loop, net::addr("220.181.112.244", 80));

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
    auto connector = std::make_shared<net::tcp_connector>(
        &loop, net::addr("220.181.112.244", 808));

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
    auto connector =
        std::make_shared<net::tcp_connector>(&loop, net::addr("0.0.0.0", 808));

    connector->set_new_conn_handler(
        [&](int fd, const errors::error_code& error) {
            EXPECT_NE(error.value(), 0);
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
    return 0;
}
