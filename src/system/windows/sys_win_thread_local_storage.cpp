#include "sys_win_thread_local_storage.h"

#include <io/io_event_loop.h>

#include <Windows.h>

#include <assert.h>
#include <thread>

using namespace std;

void *loop_curren_thread_loop() { return TlsGetValue(index); }

namespace pp {
namespace io {
static thread_local event_loop *g_loop = nullptr;
void set_current_thread_loop(event_loop *loop) { g_loop = loop; }
void clear_current_thread_loop() { g_loop = nullptr; }
bool thread_already_has_loop() { return g_loop != nullptr; }
event_loop *current_thread_loop() { return g_loop; }
} // namespace io
} // namespace pp
