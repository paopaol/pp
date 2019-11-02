#ifndef CONTAINOR_ANY_H
#define CONTAINOR_ANY_H

#include <iostream>
#include <memory>
#include <string>
#include <typeindex>

namespace pp {

struct any {
  any(void) : index_(std::type_index(typeid(void))) {}
  any(const any &that) : ptr_(that.Clone()), index_(that.index_) {}
  any(any &&that) : ptr_(std::move(that.ptr_)), index_(that.index_) {}

  //��������ָ��ʱ������һ������ͣ�ͨ��std::decay���Ƴ����ú�cv�����Ӷ���ȡԭʼ����
  template <
      typename U,
      class = typename std::enable_if<
          !std::is_same<typename std::decay<U>::type, any>::value, U>::type>
  any(U &&value)
      : ptr_(new Derived<typename std::decay<U>::type>(std::forward<U>(value))),
        index_(std::type_index(typeid(typename std::decay<U>::type))) {}

  bool is_nil() const { return !bool(ptr_); }

  template <class U> bool is() const {
    return index_ == std::type_index(typeid(U));
  }

  //��anyת��Ϊʵ�ʵ�����
  template <class U> U &any_cast() {
    if (!is<U>()) {
      throw std::bad_cast();
    }

    auto derived = dynamic_cast<Derived<U> *>(ptr_.get());
    return derived->m_value;
  }

  any &operator=(const any &a) {
    if (ptr_ == a.ptr_)
      return *this;

    ptr_ = a.Clone();
    index_ = a.index_;
    return *this;
  }

private:
  struct Base;
  typedef std::unique_ptr<Base> BasePtr;

  struct Base {
    virtual ~Base() {}
    virtual BasePtr Clone() const = 0;
  };

  template <typename T> struct Derived : Base {
    template <typename U>
    Derived(U &&value) : m_value(std::forward<U>(value)) {}

    BasePtr Clone() const { return BasePtr(new Derived<T>(m_value)); }

    T m_value;
  };

  BasePtr Clone() const {
    if (ptr_ != nullptr)
      return ptr_->Clone();

    return nullptr;
  }

  BasePtr ptr_;
  std::type_index index_;
};

} // namespace pp
#endif
