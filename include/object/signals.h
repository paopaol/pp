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

class object {
public:
	template<class func_type>
	class signal;
	template<class func_type>
	class slot {
	public:
		explicit slot(const std::function<func_type> &functor):functor_(functor) {}

	private:
		void set_connected_signal(signal<func_type> *_signal) {
			signal_ = _signal;
		
		}
		void set_slot_instace(object *instance)
		{
			revicer_ = instance;
		}

		signal<func_type> *signal_;
		object*            revicer_;
		std::function<func_type> functor_;

		friend class object;
	};

	template<class func_type>
	class signal {
	public:
		signal() {}

	private:
		void set_connected_slot(const slot<func_type> &_slot) {
			slots_.push_back(_slot);
		}
		std::vector<slot<func_type>> slots_;

		friend class object;
	};




	template<class func_type>
	static bool connect(object *sender, signal<func_type> &_signal,
		object *reciver,
		 slot<func_type> &_slot)
	{
		_signal.set_connected_slot(_slot);
		_slot.set_slot_instace(reciver);
		_slot.set_connected_signal(&_signal);
		return true;
	}


	

	template<class func_type>
	static bool connect(object *sender, signal<func_type> &_signal,
		const func_type &slot_func)
	{
		slot<func_type> _slot(slot_func);
		connect(sender, _signal, nullptr, _slot);
		return true;
	}

	template<class func_type>
	static bool connect(object *sender, signal<func_type> &_signal,
		const std::function<func_type> &slot_func)
	{
		//slot<func_type> _slot(slot_func);
		connect(sender, _signal, nullptr, slot_func);
		return true;
	}

	template<class class_type, class func_type, typename ... args_type>
	static bool connect(object *sender, signal<func_type> &_signal,
		class_type *reciver, void (class_type::*member_functor)(args_type...))
	{
		bind_member(reciver, member_functor);
		 connect(sender, _signal, 
			reciver, bind_member(reciver, member_functor));
		return  true;
	}

	template<class func_type, typename ... args_type>
	void emit(const signal<func_type> &_signal, args_type...args)
	{
		for (auto _slot : _signal.slots_) {
			_slot.functor_(args...);
		}
	}

};

}  // namespace pp
#endif /* SIGNALS_H */
