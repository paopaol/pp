#include <http_client.h>
#include <strings.h>
#include <libcurl/curl.h>


#include <functional>


namespace cstdcall{
	using namespace std;
	namespace http{
		Error::Error(const string &err)
			:m_error_string(err)
		{
		}

		const char *Error::what() const
		{
			return m_error_string.c_str();
		}






		class httpCurlClient : public Client{
		public:
			httpCurlClient()
			  :m_curl_handle(NULL),
				m_timeout(-1),
				m_curl_headers(NULL)
				
			{
				curl_global_init(CURL_GLOBAL_ALL);

				m_curl_handle = curl_easy_init();
			}

			~httpCurlClient(){
				if (m_curl_headers){
					curl_slist_free_all(m_curl_headers); /* free the header list */
				}
				curl_easy_cleanup(m_curl_handle);
			}


			void SetDeadLine(int timeout){
				m_timeout = timeout;
			}

			ResponseRef Get(const std::string &url, error_t &err){
				err = nullptr;

				RequestRef		req(new Request("GET", url));
				CURLcode		ret;
				ResponseRef		response(new Response());

				/* set URL to get */
				curl_easy_setopt(m_curl_handle, CURLOPT_URL, req->Url.c_str());

				/* no progress meter please */
				//curl_easy_setopt(m_curl_handle, CURLOPT_NOPROGRESS, 1L);
				curl_easy_setopt(m_curl_handle, CURLOPT_COOKIEFILE, "");

				//curl_easy_setopt(m_curl_handle, CURLOPT_VERBOSE, 0L);
				curl_easy_setopt(m_curl_handle, CURLOPT_WRITEFUNCTION, &httpCurlClient::getContent);
				curl_easy_setopt(m_curl_handle, CURLOPT_WRITEDATA, response.get());
				curl_easy_setopt(m_curl_handle, CURLOPT_HEADERFUNCTION, &httpCurlClient::getHeaders);
				curl_easy_setopt(m_curl_handle, CURLOPT_HEADERDATA, response.get());
				if (m_timeout > -1){
					curl_easy_setopt(m_curl_handle, CURLOPT_TIMEOUT, m_timeout);	
				}

				defer_t	do_curl_reset(nullptr, std::tr1::bind(curl_easy_reset, m_curl_handle));

				
				/* get it! */
				ret = curl_easy_perform(m_curl_handle);
				if (ret != CURLE_OK){
					err.reset(new Error(curl_easy_strerror(ret)));
					return nullptr;
				}
				response->StatusCode = getStatusCode();
				response->Request_ = req;
				response->Local.Ip = getLocalIp();
				response->Local.Port = getLocalPort();
				response->Remote.Ip = getRemoteIp();
				response->Remote.Port = getRemotePort();
				response->ContentLength = response->Content.size();
				response->Proto = getProto();

				return response;
			}

			ResponseRef Post(const std::string &url, const std::string &cotent_type, const std::string &body, error_t &err)
			{
				err = nullptr;

				RequestRef		req(new Request("POST", url));
				CURLcode		ret;
				ResponseRef		response(new Response());

				req->Content = body;
				req->ContentLength = body.size();
				req->Header[http::ContentType] = cotent_type;


				/* set URL to get */
				curl_easy_setopt(m_curl_handle, CURLOPT_URL, req->Url.c_str());

				/* Now specify the POST data */
				curl_easy_setopt(m_curl_handle, CURLOPT_POSTFIELDS, body.c_str());
				/* set the size of the postfields data */
				curl_easy_setopt(m_curl_handle, CURLOPT_POSTFIELDSIZE, body.size());

				//curl_easy_setopt(m_curl_handle, CURLOPT_VERBOSE, 1L);
				curl_easy_setopt(m_curl_handle, CURLOPT_WRITEFUNCTION, &httpCurlClient::getContent);
				curl_easy_setopt(m_curl_handle, CURLOPT_WRITEDATA, response.get());
				curl_easy_setopt(m_curl_handle, CURLOPT_HEADERFUNCTION, &httpCurlClient::getHeaders);
				curl_easy_setopt(m_curl_handle, CURLOPT_HEADERDATA, response.get());
				if (m_timeout > -1){
					curl_easy_setopt(m_curl_handle, CURLOPT_TIMEOUT, m_timeout);	
				}
				addHeaders(req->Header);


				defer_t	do_curl_reset((void *)NULL, std::tr1::bind(curl_easy_reset, m_curl_handle));
				/* get it! */
				ret = curl_easy_perform(m_curl_handle);
				if (ret != CURLE_OK){
					const char	*p = curl_easy_strerror(ret);
					err.reset(new Error(curl_easy_strerror(ret)));
					return nullptr;
				}
				response->StatusCode = getStatusCode();
				response->Request_ = req;
				response->Local.Ip = getLocalIp();
				response->Local.Port = getLocalPort();
				response->Remote.Ip = getRemoteIp();
				response->Remote.Port = getRemotePort();
				response->ContentLength = response->Content.size();
				response->Proto = getProto();

				return response;
			}


		private:
			char *getContentType(){
				char	*content_type = NULL;

				curl_easy_getinfo(m_curl_handle, CURLINFO_CONTENT_TYPE, &content_type);
				return content_type;
			}

			int getStatusCode()
			{
				long	status_code;

				curl_easy_getinfo(m_curl_handle, CURLINFO_RESPONSE_CODE, &status_code);

				return (int)status_code;
			}

			char *getLocalIp()
			{
				char	*ip = NULL;

				curl_easy_getinfo(m_curl_handle, CURLINFO_LOCAL_IP, &ip);
				return ip;
			}

			int getLocalPort()
			{
				long	port;

				curl_easy_getinfo(m_curl_handle, CURLINFO_LOCAL_PORT, &port);
				return (int)port;
			}


			char *getRemoteIp()
			{
				char	*ip = NULL;

				curl_easy_getinfo(m_curl_handle, CURLINFO_PRIMARY_IP, &ip);
				return ip;
			}

			int getRemotePort()
			{
				long	port;

				curl_easy_getinfo(m_curl_handle, CURLINFO_PRIMARY_PORT, &port);
				return (int)port;
			}

			char *getProto()
			{
				long	proto;

				curl_easy_getinfo(m_curl_handle, CURLINFO_PROTOCOL, &proto);
				switch(proto){
				case CURLPROTO_HTTP:
					return "http";
				case CURLPROTO_HTTPS:
					return "https";
				case CURLPROTO_FTP:
					return "ftp";
				case CURLPROTO_FTPS:
					return "ftps";
				default:
					return "unkown";
				}
			}

			int addHeaders(Headers &headers)
			{

				string user_agent;

				user_agent += "User-Agent:";
				user_agent += "Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 6.2; WOW64; Trident/7.0; .NET4.0E; .NET4.0C; .NET CLR 3.5.30729; .NET CLR 2.0.50727; .NET CLR 3.0.30729; HPDTDFJS)";
				m_curl_headers = curl_slist_append(m_curl_headers, user_agent.c_str());

				for (auto h = headers.begin(); h != headers.end(); h++){
					string header_content = h->first + ":" + h->second;
					m_curl_headers = curl_slist_append(m_curl_headers, header_content.c_str());
				}
				curl_easy_setopt(m_curl_handle, CURLOPT_HTTPHEADER, m_curl_headers);

				return 0;
			}



			static int  getContent(void *ptr, size_t size, size_t nmemb, void *userdata)
			{
				Response		*req = (Response *)userdata;

				req->Content.append((char *)ptr, size * nmemb);
				return size * nmemb;
			}

			static int getHeaders(void *ptr, size_t size, size_t nmemb, void *userdata)
			{
				Response		*response = (Response *)userdata;
				int				sizes = size * nmemb;
				string			h((char *)ptr, sizes);

				h = strings::TrimSpace(h);
				sizes = h.size();

				int index = h.find_first_of(":");
				if (index > 0){
					string key((char *)ptr, index);
					string val((char *)ptr + index + 1, sizes - index - 1);
					response->Header[key] = strings::TrimSpace(val);
				}else if (h.size() > 0){
					response->Status = h;
				}
				return size * nmemb;
			}


		private:
			CURL				*m_curl_handle;
			int					m_timeout;
			struct curl_slist	*m_curl_headers;

		};



		ClientRef NewClient()
		{
			ClientRef	client(new httpCurlClient());

			return client;
		}

	}
}