/**
 *   \file bxel.h
 *   \brief A Documented file.
 *
 *  Detailed description
 *
 */

#ifndef BXEL_H
#define BXEL_H

#include <atomic>
#include <filesystem>
#include <net/http/net_http_client.h>

using namespace pp;
namespace fs = std::tr2::sys;

typedef uint32_t bxel_task_id;

class bxel_task : public std::enable_shared_from_this<bxel_task> {
public:
    typedef std::function<void(bxel_task_id id, int percentage,
                               size_t                    bytes_per_second,
                               const errors::error_code& error)>
                                      progress_handler;
    typedef std::function<void(void)> cancel_done_handler;

    bxel_task(io::event_loop* loop, bxel_task_id id, int concurrent_num,
              const std::string& url, const std::string& path);

    virtual ~bxel_task() noexcept;

    // per second report once
    void on_progress(const progress_handler& handler);
    // if cancel complete, cancel_done_handler is called
    void cancel();

    void         start();
    bool         running();
    bxel_task_id id();

private:
    bxel_task(const bxel_task& other);
    bxel_task& operator=(const bxel_task& other);

    void find_range_content(net::http_response*       resp,
                            const errors::error_code& error);
    void handle_progress(const errors::error_code& error);
    void task_done(const errors::error_code& error);
    void start_download();
    void start_single_download();
    void write_file(net::http_response* resp, const errors::error_code& error);

    typedef std::shared_ptr<net::http_client> http_client_ref;
    // one block_task be responsible for a range block or the whole body
    struct block_task {
        // if block_current_ == block_end_ == 0,
        // that mean single download
        size_t                 block_current_;
        size_t                 block_end_;
        net::http_request_wref wreq_;
    };

    enum class task_state {
        kNotStarted,
        kGetRange,
        kDownloading,
        kDone,
        kError
    };

    io::event_loop*  loop_;
    net::http_client client_;
    bxel_task_id     id_;

    std::string url_;
    fs::path    path_;

    // if server is not support range-content, concurrent_numbers will be
    // overwriten to 1
    int                   concurrent_numbers_;
    std::list<block_task> block_task_list_;

    // we use below members only if support_range_ is true
    std::string            etag_;
    std::string            last_modifyed_;
    bool                   support_range_;
    progress_handler       report_progress_;
    net::http_request_wref query_req_;
    task_state             state_;
    size_t                 recved_bytes_;
    size_t                 total_bytes_;
    size_t                 speed_;
    int                    per_;
    errors::error_code     last_err_;
    FILE*                  file_;
};

class bxel {
public:
    typedef std::function<void(bxel_task_id id, int percentage,
                               size_t                    bytes_per_second,
                               const errors::error_code& error)>
                                      task_progress_handler;
    typedef std::function<void(void)> notify_handler;

    bxel(io::event_loop* loop, const std::string& name);
    virtual ~bxel() noexcept;

    void         set_concurrent_task(int number);
    void         set_progress_handler(const task_progress_handler& handler);
    bxel_task_id add_task(const std::string& url, int concurrent_numbers,
                          const std::string& path);
    void         remove_task(bxel_task_id);
    // void         cancel_task(bxel_task_id);
    // void stop_all(const notify_handler& handler);

private:
    typedef std::shared_ptr<bxel_task> bxel_task_ref;

    void add_task_in_loop(const std::string& url, int concurrent_numbers,
                          const std::string& path);
    void remove_task_in_loop(bxel_task_id id);
    void task_progress(bxel_task_id id, int per, size_t speed,
                       const errors::error_code& error);

    bxel(const bxel& other);

    bxel(bxel&& other) noexcept;

    bxel& operator=(const bxel& other);

    bxel& operator=(bxel&& other) noexcept;

    std::list<bxel_task_ref>  pending_task_list_;
    std::list<bxel_task_ref>  working_task_list_;
    io::event_loop*           loop_;
    std::string               name_;
    std::atomic<bxel_task_id> next_id_;
    int                       concurrent_tasks_;
    task_progress_handler     report_progress_;
};

#endif /* BXEL_H */
