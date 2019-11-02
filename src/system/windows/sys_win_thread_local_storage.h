#ifndef THREAD_LOCAL_STORAGE_H
#define THREAD_LOCAL_STORAGE_H

extern void set_current_thread_loop(void *loop);
extern void clear_current_thread_loop();
extern void *loop_curren_thread_loop();
extern void thread_local_storage_init();

#endif // THREAD_LOCAL_STORAGE_H
