#ifndef SIGNALS_H
#define SIGNALS_H

#include <system/sys_thread.h>
#include <io/io_event_loop.h>
#include <mutex>

namespace pp {

//---------------------------------------------------------------------
// bind_member
//---------------------------------------------------------------------
template <class Return, class Type, class... Args>
std::function<Return(Args...)> bind_member(Type* instance,
                                           Return (Type::*method)(Args...))
{
    /* 匿名函数 */
    return [=](Args&&... args) -> Return {
        /* 完美转发：能过将参数按原来的类型转发到另一个函数中 */
        /* 通过完美转发将参数传递给被调用的函数 */
        return (instance->*method)(std::forward<Args>(args)...);
    };
}

class object;

class slot;
//     signal& operator=(const signal& other);
//     signal& operator=(signal&& other) noexcept;

//     class slot<func_type>;
//     sync::Chan<slot<func_type>> slots_;
// };

class slot_private {
public:
	slot_private() {};
    virtual ~slot_private() {}
    slot_private(const slot_private&) = delete;
    slot_private& operator=(const slot_private&) = delete;
};

template <class func_type> class signal_private {
public:
	std::mutex mutex_;
    std::vector<std::shared_ptr<slot>> slots_;
};

template <class func_type> class signal {
public:
    signal() {}
	template<typename ... args_type>
	void emit(args_type... args)
	{
		std::unique_lock<std::mutex> l(mutex_);
		for (auto _slot : d_.slots_) {
			auto _slot_impl = static_cast<slot_private_impl<func_type> *>(_slot->d_.get());
			_slot_impl->func_(args...);
		}
	}

private:
    signal_private<func_type> d_;
	std::mutex                mutex_;

    friend class object;
};

template <class func_type> class slot_private_impl : public slot_private {
public:
    slot_private_impl(object* sender, signal<func_type>* _signal,
                      object* reciver, const std::function<func_type>& func)
        : sender_(sender), signal_(_signal), reciver_(reciver), func_(func)
    {
    }
    ~slot_private_impl()
    {
        // disconnect(sender_, reciver_, func_);
    }

private:
    object*                  sender_;
    signal<func_type>*       signal_;
    object*                  reciver_;
    std::function<func_type> func_;

	friend class signal<func_type>;
};

class slot {
public:
    template <class func_type>
    explicit slot(object* sender, signal<func_type>* _signal, object* reciver,
                  const std::function<func_type>& func)
        : d_(std::make_shared<slot_private_impl<func_type>>(sender, _signal,
                                                            reciver, func))
    {
    }

public:
    std::shared_ptr<slot_private> d_;
    friend class object;
};

class object {
public:
    object(io::event_loop* loop):loop_(loop){}
	virtual ~object() noexcept {};
    system::thread_id thread();

    template <class func_type>
    static bool connect(object* sender, signal<func_type>* _signal,
                        const std::function<func_type>& func)
    {
        std::shared_ptr<slot> _slot =
            std::make_shared<slot>(sender, _signal, nullptr, func);
        _signal->d_.slots_.push_back(_slot);
        return true;
    }

    // template <class ins_type, class func_type>
    // static bool connect(object* sender, signal<func_type>* _signal,
    //	ins_type reciver, func_type func)
    //{
    //	return bind_member(reciver, func);
    //}

private:
    object(const object& other);
    object(object&& other) noexcept;
    object& operator=(const object& other);
    object& operator=(object&& other) noexcept;

    io::event_loop* loop_;
};
//
//#define def_signal(scope, signal_name, args...) \
//    scope:                                      \
//    signal<args> signal_name;
//

}  // namespace pp
#endif /* SIGNALS_H */
