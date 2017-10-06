#ifndef CSTDCALL_HTTP_CLIENT_H
#define CSTDCALL_HTTP_CLIENT_H

#include <string>
#include <map>
#include <memory>
#include <exception>

#include "cstdcall.h"

#include "cstdcall_export.h"

#include "net.h"


#pragma warning( disable: 4251 )

namespace cstdcall{




	// a simple http client tool, based on libcurl
	//see https://curl.haxx.se


	namespace http{
		const auto ContentType = "Content-Type";


		const auto ContentTypeJson = "application/json";





		class CSTDCALL_API Error : public std::exception{
		public:
			Error(const std::string &err);
			const char *what() const;

		private:
			std::string m_error_string;
		};




		CSTDCALL_API typedef std::map<std::string, std::string> Headers;
		struct Response;
		struct Request;
		class Client;
		CSTDCALL_API typedef std::tr1::shared_ptr<Response>		ResponseRef;
		CSTDCALL_API typedef std::tr1::shared_ptr<Request>		RequestRef;
		CSTDCALL_API typedef std::tr1::shared_ptr<Client>					ClientRef;


		struct CSTDCALL_API Request{
		public:
			Request(const std::string &method, const std::string &url);

			std::string				Method;	
			std::string				Url;
			int						ContentLength;
			std::string				Content;
			Headers					Header;
		};

		struct CSTDCALL_API Response{
		public:
			std::string				Status;
			int						StatusCode;
			std::string				Proto;				//http, https, ftp, ftps
			Headers					Header;
			int						ContentLength;
			std::string				Content;
			RequestRef				Request_;
			net::Addr				Local;
			net::Addr				Remote;
		};



		/*simple http client,
		 *please use NewClient() to create one http client;
		*/
		class CSTDCALL_API Client{
		public:
			~Client(){};


			//Do GET method
			//if any error occur, Get() will set error in err,
			//and the return value is nullptr;
			virtual ResponseRef Get(const std::string &url, error_t &err) = 0;



			//Do POST method
			//@url:the url for post
			//@content_type:the content_type, such as application/json;
			//@body:body content
			//if any error occur, Post() will set error in err, and the return value is nullptr;
			virtual ResponseRef Post(const std::string &url, const std::string &cotent_type, const std::string &body, error_t &err) = 0;


			//set maximum time the request is allowed to take 
			//Default timeout is 0 (zero) which means it never times out during transfer. 
			virtual void SetDeadLine(int timeout /*seconds*/) = 0;

		};


		//create one http client
		CSTDCALL_API ClientRef NewClient();

	}
}

#endif