# PP #
pp is a c++ network frameworks.

## support platforms ##
  * [x] windows(iocp) 
  * [ ] linux(epoll)
  * [ ] freebsd(kqueue)
  
## functions ##
  * [x] tcp_server
  * [x] tcp_client
  * [ ] udp_server
  * [ ] udp_client
  * [ ] http_server
  * [x] http_client (currently only GET )
  * [x] timer(timer heap)
  * [ ] timer(timer wheel)
  * [x] better error reporter
  
### tcp/udp connection ###
  * [x] write finished signal
  * [ ] write buffer high water signal

### multithread ###
  * [ ] multithread

### compiler environment  ###
| platform | compiler         |
|----------|------------------|
| windows  | msvc2015 update3 |

### examples
  * [ ] http downloader
  * [x] http client
  * [ ] tcp echo server
  * [ ] tcp echo client
  * [ ] socks5 server

