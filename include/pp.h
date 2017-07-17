#ifndef PP_H
#define PP_H



#include <memory>
#include <vector>
#include <string>
#include <functional>



namespace pp{
	typedef std::shared_ptr<void>	Defer;
	typedef std::vector<char> 		Slice;


}

#include "errors.h"
#include "io.h"
#include "_time.h"
#include "buffer.h"


#endif