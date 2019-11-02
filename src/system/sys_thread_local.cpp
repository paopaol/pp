#include <system/sys_thread_local.h>

namespace pp {
namespace system {
thread_local io::event_loop *g_loop = nullptr;
void set_current_thread_loop(io::event_loop *loop) { g_loop = loop; }
void clear_current_thread_loop() { g_loop = nullptr; }
bool thread_already_has_loop() { return g_loop != nullptr; }
io::event_loop *current_thread_loop() { return g_loop; }
} // namespace system
} // namespace pp
