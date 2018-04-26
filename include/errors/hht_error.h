#ifndef HHT_ERROR_H
#define HHT_ERROR_H

#include <system_error>
#include <vector>

#include <typeinfo>
#include <typeindex>
#include <stdio.h>
#include <sstream>
#include <iostream>
#include <tuple>


namespace pp{
    

namespace errors {






    /**
     * \class   error_code
     *
     * \brief   An error code.
     *
     * \author  Jinzh
     * \date    2018/3/26
     */
    class error_code :public std::error_code {
    public:
        error_code(const std::error_code &code)
            :std::error_code(code)
        {
        }

        error_code()
            :std::error_code()
        {
        }

        void  prefix_msg(const std::string &prefix)
        {
            prefix_msg_ = prefix;
        }



        void suffix_msg(const std::string &suffix)
        {
            suffix_msg_ = suffix;
        }

        void push_call_stack(const std::string &file, const std::string &func, int line)
        {
            std::stringstream s;

            s << "  at " <<file << ":" << line << ")";
            call_stack_.push_back(s.str());
        }


        std::string message()const {
            std::string msg;

            if (!prefix_msg_.empty()) {
                msg = prefix_msg_ + "  ";
            }
            msg += std::error_code::message();
            if (!suffix_msg_.empty()) {
                msg += "   " + suffix_msg_;
            }
            return msg;
        }

        std::string full_message() {
            std::string msg = message();
            for (auto line = call_stack_.begin(); line != call_stack_.end(); line++) {
                msg += (std::string("\n") + *line);
            }
            return msg;
        }

        template<class _Enum>
        typename std::enable_if<std::is_error_code_enum<_Enum>::value,
            error_code>::type& operator=(_Enum _Errcode)
        {	// assign enumerated error code
            *this = std::make_error_code(_Errcode);	// using ADL
            return (*this);
        }

    private:
        std::string prefix_msg_;
        std::string suffix_msg_;
        std::vector<std::string> call_stack_;
        std::string enum_value_;
    };

    template<class _Enum>
    error_code  make_error_code(_Enum code, const std::string &file, 
                    const std::string &function, int line) {
        error_code error = std::make_error_code(code);
        error.push_call_stack(file, function, line);
        return error;
    }

#define __enum_to_str__(enum_val) #enum_val


#define format_error_message(buffer, enum_class_code, message_c_str)   snprintf(buffer, sizeof(buffer) - 1, "[%s %s] %s", typeid(enum_class_code).name(), __enum_to_str__(enum_class_code), message_c_str);


//构造error_code
#define hht_make_error_code(code, ...) errors::make_error_code((code), __FILE__, __FUNCTION__, __LINE__)
//push函数调用堆栈
#define hht_push_call_stack(error, ...) (error).push_call_stack(__FILE__, __FUNCTION__, __LINE__)

//检查error并且返回指定code
#define hht_return_if_error(error, return_, ...) {if ((error).value() != 0) {hht_push_call_stack((error)); return (return_);}}
#define hht_return_none_if_error(error, ...) {if ((error).value() != 0) {hht_push_call_stack((error)); return ;}}
//检查error，在返回之前调用functor
#define hht_return_with_functor_if_error(error, return_, functor, ...) {if (error.value() != 0) {hht_push_call_stack((error)); {functor();}; return (return_);}}
#define hht_return_none_with_functor_if_error(error, functor, ...) {if (error.value() != 0) {hht_push_call_stack((error)); {functor();}; return ;}}


}
}
#endif
