#ifndef THREAD_LOCAL_STORAGE_H
#define THREAD_LOCAL_STORAGE_H

extern void loopPushToThread(void *loop);
extern void loopPopFromThread();
extern void *loop_curren_thread_loop();
extern void thread_local_storage_init();

#endif // THREAD_LOCAL_STORAGE_H
