
#include <pp.h>

#include <vector>
#include <string>
#include <iterator>
#include <algorithm>
#include <assert.h>
#include <memory>

namespace pp{
    namespace bytes{
        static const auto errorOutOfRange = errors::Error("Buffer out of range");





        Buffer::Buffer()
        :b(4096),
        ridx(0),
        widx(0)
        {
        }

        //Read All
        size_t Buffer::Read(std::vector<char> &p)
        {
            return ReadBytes(p, Len());
        }

        //Read One Byte
        char Buffer::ReadByte()
        {
            char *ch = lastRead();
            hasReaded(1);
            return *ch;
        }

        //Read N Bytes from buffer
        size_t Buffer::ReadBytes(std::vector<char> &p, int n)
        {
            assert(n > 0);

            p.clear();
            n = n > Len() ? Len() : n;
            std::copy(lastRead(), lastRead() + n, std::back_inserter(p));
            hasReaded(n);
            return n;
        }

        size_t Buffer::Read(char *buffer, int n)
        {
            assert(n > 0);
            n = n > Len() ? Len() : n;
            std::copy(lastRead(), lastRead() + n, buffer);
            hasReaded(n);
            return n;
        }

        //write data into buffer
        size_t Buffer::Write(const char *d, int len)
        {
            if (leftSpace() < len){
                growSpace(len);
            }
            std::copy(d, d + len, beginWrite());
            hasWritten(len);
            return len;
        }
        size_t Buffer::Write(const std::string &s)
        {
            return Write(s.c_str(), s.size());
        }

        size_t Buffer::Write(const std::vector<char> &p)
        {
            return Write(p.data(), p.size());
        }

        bool Buffer::UnReadByte(errors::Error &e)
        {
           return UnReadBytes(1, e);
        }

        bool Buffer::UnReadBytes(int n, errors::Error &e)
        {
            auto len = lastRead() - begin();
            if (len < n){
                e = errorOutOfRange;
                return false;
            }
            ridx -= n;
            return false;
        }

        //return unreaded data size
        int Buffer::Len()
        {
            return widx - ridx;
        }

        int Buffer::Cap()
        {
            return b.capacity();
        }

        void Buffer::Reset()
        {
            ridx = 0;
            widx = 0;
        }

        void Buffer::Optimize()
        {
            if (ridx == 0){
                return;
            }
            int len = Len();
            std::copy(lastRead(), beginWrite(), begin());
            ridx = 0;
            widx = ridx + len;
        }
        //ReadFrom
        //WriteTo


        void Buffer::growSpace(int len)
        {
            b.resize(widx + len);
        }


        int Buffer::leftSpace()
        {
            return b.capacity() - widx;
        }

        void Buffer::hasWritten(int len)
        {
            widx += len;
        }
        void Buffer::hasReaded(int len)
        {
            ridx += len;
        }

        char *Buffer::beginWrite()
        {
            return begin() + widx;
        }

        const char *Buffer::beginWrite()const
        {
            return begin() + widx;
        }

        char *Buffer::lastRead()
        {
            return begin() + ridx;
        }

        const char *Buffer::beginRead() const
        {
            return begin() + ridx;
        }

        char *Buffer::begin()
        {
            return &*b.begin();
        }

        const char *Buffer::begin()const
        {
            return &*b.begin();
        }

        BufferRef NewBuffer()
        {
            BufferRef b = std::make_shared<Buffer>();
            return b;
        }
    }
}
