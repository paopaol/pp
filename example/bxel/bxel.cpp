/**
 *   \file bxel.cpp
 *   \brief A Documented file.
 *
 * bxel is a http downloader, is like axel too under linux.
 *
 * 1. concurrent download
 * 2. resume download if connection is broken
 */

#include "bxel.h"
#include <fmt/fmt.h>
#include <io/io_event_loop.h>
#include <strings/strings.h>

using namespace std::tr1::placeholders;

#define make_net_error(e, msg)                                 \
    e = hht_make_error_code(                                   \
        static_cast<errors::error>(errors::error::NET_ERROR)); \
    e.suffix_msg((msg));

bxel_task::bxel_task(io::event_loop* loop, bxel_task_id id, int concurrent_num,
                     const std::string& url, const std::string& path)
    : loop_(loop),
      client_(loop_),
      id_(id),
      url_(url),
      path_(path),
      concurrent_numbers_(concurrent_num),
      support_range_(false),
      state_(task_state::kNotStarted),
      recved_bytes_(0),
      total_bytes_(-1),
      file_(nullptr)
{
}

bxel_task::~bxel_task()
{
    assert(state_ == task_state::kDone);
}

void bxel_task::on_progress(const download_progress_handler& handler)
{
    report_progress_ = handler;
}

void bxel_task::cancel()
{
    assert(loop_->in_created_thread());
    auto req = query_req_.lock();
    if (req) {
        client_.cancel(req);
    }

    for (auto btask = block_task_list_.begin(); btask != block_task_list_.end();
         btask++) {
        auto req = btask->wreq_.lock();
        if (req) {
            client_.cancel(req);
        }
    }
}

static size_t try_get_content_len(const net::http_response* resp)
{
    auto header = resp->headers.find("Content-Range");
    if (header != resp->headers.end()) {
        // Content-Range: bytes 0-499/22400
        std::vector<std::string> range;
        range = strings::Split(range, header->second, "/");
        return range.size() == 2 ? atoi(range[1].c_str()) : 0;
    }
    header = resp->headers.find("Content-Length");
    if (header != resp->headers.end()) {
        return atoi(header->second.c_str());
    }
    return -1;
}

static std::string try_get_etag(const net::http_response* resp)
{
    auto header = resp->headers.find("Etag");
    if (header != resp->headers.end()) {
        return header->second;
    }
    return "";
}

static std::string try_get_last_modify(const net::http_response* resp)
{
    auto header = resp->headers.find("Last-Modified");
    if (header != resp->headers.end()) {
        return header->second;
    }
    return "";
}

void bxel_task::handle_progress(const errors::error_code& error)
{
    if (report_progress_) {
        report_progress_(id_, recved_bytes_, total_bytes_, error);
    }
}

void bxel_task::task_done(const errors::error_code& error)
{
    state_ = task_state::kDone;
    loop_->run_in_loop(
        std::bind(&bxel_task::handle_progress, shared_from_this(), error));
}

static void use_new_filename(fs::path& path)
{
    for (int id = 0; id < 100000; id++) {
        fs::path test_path;
        auto     ext      = path.extension();
        auto     parent   = path.parent_path();
        auto     filename = path.stem();
        if (path.has_parent_path()) {
            test_path.append(parent);
        }
        if (path.has_stem()) {
            test_path.append(filename);
        }
        test_path += fmt::Sprintf(" (%d)", id);
        if (path.has_extension()) {
            test_path += ext;
        }
        if (!fs::exists(test_path)) {
            path = test_path;
            return;
        }
    }
}

static std::shared_ptr<FILE> create_file_if_no_exit(fs::path&           path,
                                                    errors::error_code& error)
{
    if (path.has_parent_path()) {
        auto parent = path.parent_path();
        fs::create_directories(parent, error);
        if (error) {
            return nullptr;
        }
    }
    if (fs::exists(path)) {
        use_new_filename(path);
    }
    FILE* fp = fopen(path.string().c_str(), "ab");
    if (!fp) {
        error = hht_make_error_code(
            static_cast<std::errc>(std::errc::permission_denied));
        error.prefix_msg(path.string());
        return nullptr;
    }
    std::shared_ptr<FILE> f(fp, fclose);
    return f;
}

void bxel_task::write_file(net::http_response* resp, bool finished,
                           const errors::error_code& error)
{
    if (error) {
        task_done(error);
        return;
    }
    if (finished) {
        if (!last_err_) {
            // success done
            total_bytes_ = recved_bytes_;
        }
        task_done(last_err_);
        return;
    }

    int status_code = resp->status_code;
    if (status_code != 200) {
        make_net_error(last_err_, fmt::Sprintf("%d %s", status_code,
                                               resp->status_line.c_str()));
        return;
    }

    file_ = create_file_if_no_exit(path_, last_err_);
    if (last_err_) {
        return;
    }
    resp->body =
        io::writer([&](const char* buf, size_t len, errors::error_code& error) {
            // error = hht_make_error_code(
            //     static_cast<std::errc>(std::errc::no_space_on_device));
            fwrite(buf, 1, len, file_.get());
            recved_bytes_ += len;
            return len;
        });
}

void bxel_task::start_single_download()
{
    block_task sub_task;

    sub_task.block_current_ = 0;
    sub_task.block_end_     = 0;

    auto req = client_.new_request(
        url_, net::http_method::kGet,
        std::bind(&bxel_task::write_file, shared_from_this(), _1, _2, _3));
    sub_task.wreq_ = req;
    block_task_list_.push_back(sub_task);
    errors::error_code err;
    client_.run(req, err);
    if (err) {
        task_done(err);
    }
}

void bxel_task::start_download()
{
    int concurrent_num = support_range_ ? concurrent_numbers_ : 1;
    concurrent_num     = 1;
    if (concurrent_num == 1) {
        start_single_download();
        return;
    }
    // start_mutiple_download();
}

void bxel_task::find_range_content(net::http_response* resp, bool finished,
                                   const errors::error_code& error)
{
    if (error) {
        task_done(error);
        return;
    }

    if (finished) {
        if (last_err_) {
            task_done(last_err_);
            return;
        }
        assert(state_ == task_state::kDownloading);
        start_download();
        return;
    }

    // uri not found
    int status_code = resp->status_code;
    if (status_code != 200 && status_code != 206) {
        make_net_error(last_err_, fmt::Sprintf("%d %s", status_code,
                                               resp->status_line.c_str()));
        return;
    }

    support_range_ = resp->status_code == 206 ? true : false;
    total_bytes_   = try_get_content_len(resp);
    if (total_bytes_ == -1) {
        support_range_ = false;
    }
    etag_          = try_get_etag(resp);
    last_modifyed_ = try_get_last_modify(resp);
    state_         = task_state::kDownloading;
}

void bxel_task::start()
{
    assert(loop_->in_created_thread());

    if (state_ != task_state::kNotStarted) {
        return;
    }
    state_ = task_state::kGetRange;

    errors::error_code err;

    auto req              = client_.new_request(url_, net::http_method::kGet,
                                   std::bind(&bxel_task::find_range_content,
                                             shared_from_this(), _1, _2, _3));
    req->headers["Range"] = "bytes=0-9";
    query_req_            = req;
    client_.run(req, err);
    if (err) {
        state_ = task_state::kDone;
        task_done(err);
    }
    return;
}

bxel_task_id bxel_task::id()
{
    return id_;
}

/////////////////////////////////////////////////

bxel::bxel(io::event_loop* loop, const std::string& name)
    : loop_(loop), name_(name), concurrent_tasks_(1), next_id_(0)
{
}

bxel::~bxel() {}

void bxel::task_progress(bxel_task_id id, size_t bytes_recved,
                         size_t bytes_total, const errors::error_code& error)
{
    if (report_progress_) {
        report_progress_(id, bytes_recved, bytes_total, error);
    }
    if (error || bytes_recved == bytes_total) {
        remove_task(id);
    }
}

void bxel::set_concurrent_task(int number)
{
    number            = std::max(1, number);
    number            = std::min(100, number);
    concurrent_tasks_ = number;
}

void bxel::set_progress_handler(
    const bxel_task::download_progress_handler& handler)
{
    report_progress_ = handler;
}

void bxel::remove_task(bxel_task_id id)
{
    loop_->run_in_loop(std::bind(&bxel::remove_task_in_loop, this, id));
}

void bxel::remove_task_in_loop(bxel_task_id id)
{
    assert(loop_->in_created_thread());

    // try remove form pending list
    for (auto task = pending_task_list_.begin();
         task != pending_task_list_.end(); task++) {
        if ((*task)->id() == id) {
            pending_task_list_.erase(task);
            return;
        }
    }

    // try remove from working list
    for (auto task = working_task_list_.begin();
         task != working_task_list_.end(); task++) {
        if ((*task)->id() == id) {
            working_task_list_.erase(task);
            return;
        }
    }
}

bxel_task_id bxel::add_task(const std::string& url, int concurrent_numbers,
                            const std::string& path)
{
    auto id = next_id_++;
    loop_->run_in_loop(std::bind(&bxel::add_task_in_loop, this, url,
                                 concurrent_numbers, path, id));
    return id;
}

void bxel::add_task_in_loop(const std::string& url, int concurrent_numbers,
                            const std::string& path, bxel_task_id id)
{
    assert(loop_->in_created_thread());
    auto task =
        std::make_shared<bxel_task>(loop_, id, concurrent_numbers, url, path);

    task->on_progress(std::bind(&bxel::task_progress, this, _1, _2, _3, _4));

    if (working_task_list_.size() == concurrent_tasks_) {
        pending_task_list_.push_back(task);
        return;
    }
    working_task_list_.push_back(task);
    task->start();
}

static io::event_loop loop;

static void report_progress(bxel_task_id id, size_t bytes_recved,
                            size_t bytes_total, const errors::error_code& error)
{
    printf("task:%d\tbytes recved: %zd\tbytes total:%zd\t%s\n", id,
           bytes_recved, bytes_total, error.message().c_str());
    loop.quit();
}

// this is a single thread application
int main(int argc, char* argv[])
{
    bxel bxel_(&loop, "bxel is a http downloader");

    bxel_.set_concurrent_task(1);
    bxel_.set_progress_handler(std::bind(report_progress, _1, _2, _3, _4));

    // one way
    int         concurrent_numbers = atoi(argv[1]);
    std::string url                = argv[2];
    std::string path               = argv[3];

    // second way
    fs::path task_file = argv[1];

    bxel_.add_task(url, concurrent_numbers, path);

    loop.exec();

    return 0;
}
