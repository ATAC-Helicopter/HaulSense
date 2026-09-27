#include "dashboard.hpp"
#include "dashboard_asset.hpp"
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <charconv>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

Dashboard::~Dashboard(){for(auto&c:clients_)close(c);if(listener_>=0)::close(listener_);}
void Dashboard::close(Client& c){if(c.fd>=0)::close(c.fd);c=Client{};}
bool Dashboard::open(unsigned short port){
    port_=port;
    listener_=socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0);if(listener_<0)return false;
    int one=1;setsockopt(listener_,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one));
    sockaddr_in a{};a.sin_family=AF_INET;a.sin_port=htons(port);a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if(bind(listener_,(sockaddr*)&a,sizeof(a))||listen(listener_,8)){::close(listener_);listener_=-1;return false;}return true;
}
void Dashboard::tick(const Handler& handler){
    if(listener_<0)return;
    // Bounded work, no blocking client may stall the controller's feedback loop.
    for(unsigned n=0;n<8;++n){
        int fd=accept4(listener_,nullptr,nullptr,SOCK_NONBLOCK|SOCK_CLOEXEC);if(fd<0)break;
        auto it=std::find_if(clients_.begin(),clients_.end(),[](const Client&c){return c.fd<0;});
        if(it==clients_.end()){::close(fd);continue;}it->fd=fd;it->start=std::chrono::steady_clock::now();
    }
    for(auto&c:clients_){
        if(c.fd<0)continue;
        if(std::chrono::steady_clock::now()-c.start>std::chrono::seconds(2)){close(c);continue;}
        if(c.output.empty()){
            char buffer[4096];auto count=recv(c.fd,buffer,sizeof(buffer),0);
            if(count==0||(count<0&&errno!=EAGAIN&&errno!=EWOULDBLOCK)){close(c);continue;}
            if(count>0)c.input.append(buffer,count);
            if(c.input.size()>8192){close(c);continue;}
            auto end=c.input.find("\r\n\r\n");if(end==std::string::npos)continue;
            auto first=c.input.find("\r\n");std::string request=c.input.substr(0,first);
            std::string headers=c.input.substr(first+2,end-first-2);
            std::transform(headers.begin(),headers.end(),headers.begin(),[](unsigned char v){return std::tolower(v);});
            // Reject DNS rebinding and cross-origin writes. No CORS access is granted.
            bool host=false;
            std::string header_lines="\r\n"+headers+"\r\n";
            for(const auto&name:{"127.0.0.1","localhost"})if(header_lines.find(std::string("\r\nhost: ")+name+":"+std::to_string(port_)+"\r\n")!=std::string::npos)host=true;
            int status=200;std::string body,type="application/json";
            bool post=request=="POST /api/config HTTP/1.1";
            size_t length=0;
            if(post){
                auto at=headers.find("content-length: ");
                if(at==std::string::npos){status=400;}
                else {at+=16;auto finish=headers.find("\r\n",at);auto val=headers.substr(at,finish-at);auto r=std::from_chars(val.data(),val.data()+val.size(),length);if(r.ec!=std::errc{}||r.ptr!=val.data()+val.size()||length>2048)status=400;}
                if(status==200&&c.input.size()<end+4+length)continue;
                if(headers.find("x-haulsense: 1")==std::string::npos)status=403;
            }
            if(!host)status=403;
            if(status==200){
                if(request=="GET / HTTP/1.1"){body=dashboard_html;type="text/html; charset=utf-8";}
                else if(request=="GET /api/state HTTP/1.1")body=handler("state","",status);
                else if(post)body=handler("config",c.input.substr(end+4,length),status);
                else status=404;
            }
            if(status!=200&&body.empty())body="{\"error\":\"Request rejected\"}";
            c.output="HTTP/1.1 "+std::to_string(status)+" "+(status==200?"OK":"Error")+"\r\nContent-Type: "+type+"\r\nContent-Length: "+std::to_string(body.size())+"\r\nConnection: close\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nX-Frame-Options: DENY\r\nContent-Security-Policy: default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; img-src 'self' data:; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'\r\n\r\n"+body;
        }
        auto count=send(c.fd,c.output.data()+c.sent,c.output.size()-c.sent,MSG_NOSIGNAL);
        if(count<0){if(errno!=EAGAIN&&errno!=EWOULDBLOCK)close(c);continue;}
        c.sent+=count;if(c.sent==c.output.size())close(c);
    }
}
