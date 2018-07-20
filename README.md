# PP #
pp is a c++ network frameworks.

## support platforms ##
  * [x] windows(iocp) 
  * [ ] linux(epoll)
  * [ ] freebsd(kqueue)
  
## functions ##
  * [x] tcp_server
  * [ ] tcp_client
  * [ ] udp_server
  * [ ] udp_client
  * [ ] http_server
  * [ ] http_client
  * [x] timer(timer heap)
  * [ ] timer(timer circle)
  * [ ] better error reporter
  
### tcp/udp connection ###
  * [x] write finished signal
  * [ ] write buffer high water signal

### multithread ###
  * [ ] multithread

### compiler environment  ###
| platform | compiler         |
|----------|------------------|
| windows  | msvc2015 update3 |
