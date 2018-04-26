#ifndef HHT_BYTES_BUFFER_H
#define HHT_BYTES_BUFFER_H
#include <vector>
#include <string>
#include <iterator>
#include <algorithm>
#include <assert.h>
#include <memory>


namespace pp {
    namespace bytes {
        class Buffer;
        typedef std::shared_ptr<Buffer> BufferRef;

        class Buffer {
        public:
            //friend BufferRef  NewBuffer();


            Buffer();
            ~Buffer() { printf("ddddddddddddddddddd\n"); }
          

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
           

            void UnReadByte(/*error*/);
           

            void UnReadBytes(int n/*,error &e*/);
            


            //return unreaded data size
            int Len();
           

            int Cap();
            
            void Reset();
          

            bool PeekAt(std::vector<char> &p, int index, int size);
           

            void Optimization();


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

        //BufferRef NewBuffer();
      //  {
            //BufferRef b = std::make_shared<Buffer>();
            //return b;
      //  }
    }
}

#endif