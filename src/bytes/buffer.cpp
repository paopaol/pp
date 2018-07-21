#include <bytes/buffer.h>

namespace pp {
namespace bytes {

    Buffer::Buffer() : b(4096), ridx(0), widx(0) {}

    // Read All
    size_t Buffer::Read(std::vector<char>& p)
    {
        return ReadBytes(p, Len());
    }

    // Read One Byte
    char Buffer::ReadByte()
    {
        char* ch = lastRead();
        hasReaded(1);
        return *ch;
    }

    // Read N Bytes from buffer
    size_t Buffer::ReadBytes(std::vector<char>& p, int n)
    {
        assert(n >= 0 && "buffer::readbytes(), bad input paramer");

        p.clear();
        n = n > Len() ? Len() : n;
        std::copy(lastRead(), lastRead() + n, std::back_inserter(p));
        hasReaded(n);
        return n;
    }

    size_t Buffer::Read(char* buffer, int n)
    {
        assert(n >= 0 && "buffer::read(), bad input paramer");
        n = n > Len() ? Len() : n;
        std::copy(lastRead(), lastRead() + n, buffer);
        hasReaded(n);
        return n;
    }

    // write data into buffer
    size_t Buffer::Write(const char* d, int len)
    {
        if (leftSpace() < len) {
            Optimization();
            growSpace(b.size() + len);
        }
        std::copy(d, d + len, beginWrite());
        hasWritten(len);
        return len;
    }
    size_t Buffer::Write(const std::string& s)
    {
        return Write(s.c_str(), s.size());
    }

    size_t Buffer::Write(const std::vector<char>& p)
    {
        return Write(p.data(), p.size());
    }

    void Buffer::UnReadByte(/*error*/)
    {
        UnReadBytes(1);
    }

    void Buffer::UnReadBytes(int n /*,error &e*/)
    {
        assert((lastRead() - begin()) >= n
               && "buffer::unreadbytes too much data size");
        ridx -= n;
    }

    // return unreaded data size
    int Buffer::Len()
    {
        return widx - ridx;
    }

    int Buffer::Cap()
    {
        return b.size();
    }

    void Buffer::Reset()
    {
        ridx = 0;
        widx = 0;
    }

    bool Buffer::PeekAt(std::vector<char>& p, int index, int size)
    {
        if (index < 0 || index >= Len()) {
            return false;
        }
        if (size <= 0) {
            return false;
        }
        index   = ridx + index;
        int len = widx - index;
        if (size > len) {
            return false;
        }

        p.clear();
        std::copy(b.data() + index, b.data() + index + size,
                  std::back_inserter(p));
        return true;
    }

    void Buffer::Optimization()
    {
        if (ridx == 0) {
            return;
        }

        int len = Len();
        std::copy(begin() + ridx, begin() + widx, begin());
        ridx = 0;
        widx = ridx + len;
        assert(widx < b.size());
    }

    // ReadFrom
    // WriteTo

    void Buffer::growSpace(int len)
    {
        b.resize(widx + len);
    }

    int Buffer::leftSpace()
    {
        return b.size() - widx;
    }

    void Buffer::hasWritten(int len)
    {
        widx += len;
        assert(widx <= b.size());
    }
    void Buffer::hasReaded(int len)
    {
        ridx += len;
    }

    char* Buffer::beginWrite()
    {
        return begin() + widx;
    }

    const char* Buffer::beginWrite() const
    {
        return begin() + widx;
    }

    char* Buffer::lastRead()
    {
        return begin() + ridx;
    }

    const char* Buffer::beginRead() const
    {
        return begin() + ridx;
    }

    char* Buffer::begin()
    {
        return &*b.begin();
    }

    const char* Buffer::begin() const
    {
        return &*b.begin();
    }

    // BufferRef NewBuffer();
    //  {
    // BufferRef b = std::make_shared<Buffer>();
    // return b;
    //  }
}  // namespace bytes
}  // namespace pp

#if 0
int main(int argc ,char *argv[])
{
	auto buffer = bytes::NewBuffer();
	std::vector<char> data(10000, 1);

	buffer->Write("12345");
	buffer->Write("12345");
	buffer->Write("12345");
	buffer->Write("12345");
	buffer->Write(data);

	printf("cap:%d\n len:%d\n", buffer->Cap(), buffer->Len());
	buffer->PeekAt(data, 3, 10);

	printf("cap:%d\n len:%d\n", buffer->Cap(), buffer->Len());
	buffer->ReadByte();
	buffer->PeekAt(data, 3, 10);
	buffer->ReadByte();
	buffer->ReadByte();
	buffer->ReadByte();
	buffer->PeekAt(data, 3, 10);
	buffer->Optimization();
	buffer->PeekAt(data, 3, 10);
	buffer->ReadBytes(data, 22);
	buffer->UnReadByte();
	buffer->UnReadByte();
	buffer->UnReadByte();
	buffer->UnReadByte();
	buffer->UnReadBytes(22);
	printf("cap:%d\n len:%d\n", buffer->Cap(), buffer->Len());

    return 0;
}

#endif
