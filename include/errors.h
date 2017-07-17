#ifndef PP_ERRORS_H
#define PP_ERRORS_H
#include <string>
#include <memory>


namespace pp{
    namespace errors{
        class Error{
        public:
            Error(){}

            Error(const std::string &s)
                :e(std::make_shared<std::string>(s))
            {
            }
            const char *what() const
            {
                return e->c_str();
            }

            bool operator == (const Error &e1)
            {
                return e1.e == e;
            }

            friend Error New(const std::string &s);

        private:
            typedef std::shared_ptr<std::string> String;


            mutable String    e;
        };


        Error New(const std::string &s);
    }
}

#endif