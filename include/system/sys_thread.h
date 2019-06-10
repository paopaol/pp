#ifndef HHT_THREAD_H
#define HHT_THREAD_H

namespace pp {
namespace system {
    typedef unsigned int thread_id;
    namespace this_thread {

        thread_id get_id();
    }

}  // namespace system
}  // namespace pp

#endif
