#ifndef PP_BUFFER_H
#define PP_BUFFER_H

#include "pp.h"
#include <vector>
#include <string>
#include <iterator>
#include <algorithm>
#include <assert.h>
#include <memory>

namespace pp{
    namespace bytes{
        class Buffer;
        typedef std::shared_ptr<Buffer> BufferRef;

        BufferRef NewBuffer();

        class Buffer{
        public:
            friend BufferRef  NewBuffer();

            Buffer();
            //Read All
            size_t Read(std::vector<char> &p);

            //Read One Byte
            char ReadByte();

            //Read N Bytes from buffer
            size_t ReadBytes(std::vector<char> &p, int n);
            

            size_t Read(char *buffer, int n);
            

            //write data into buffer
            size_t Write(const char *d, int len);
            
            size_t Write(const std::string &s);
            

            size_t Write(const std::vector<char> &p);
            

            bool UnReadByte(errors::Error &e);
            

            bool UnReadBytes(int n, errors::Error &e);
            

            //return unreaded data size
            int Len();
    

            int Cap();
    

            void Reset();

            void Optimize();
            

            //ReadFrom
            //WriteTo

        private:

            void growSpace(int len);
    


            int leftSpace();
            

            void hasWritten(int len);
            
            void hasReaded(int len);
            

            char *beginWrite();
            

            const char *beginWrite()const;
    

            char *lastRead();
            

            const char *beginRead() const;
            

            char *begin();
            

            const char *begin()const;
    

            std::vector<char>			b;
            size_t                      ridx;
            size_t                      widx;   
        };
    }
}

#endif