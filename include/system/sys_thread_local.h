#ifndef __THREAD_LOCAL_H_
#define __THREAD_LOCAL_H_

#include <assert.h>
#include <io/io_event_loop.h>

namespace pp {
namespace system {
void set_current_thread_loop(io::event_loop *loop);
void clear_current_thread_loop();
bool thread_already_has_loop();
io::event_loop *current_thread_loop();
} // namespace system
} // namespace pp

#endif // __THREAD_LOCAL_H_
