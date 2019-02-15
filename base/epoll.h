#ifndef __EPOLL_H__
#define __EPOLL_H__
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

class EpollServer
{
public:
    EpollServer(int port)
        :port_(port)
        ,listen_sock_(-1)
        ,epollfd_(-1)
    {}
    virtual ~EpollServer()
    {
        if(listen_sock_)
            close(listen_sock_);
    }
    void Start();
    void EventLoop();
//    void EventHandler(struct epoll_event & event);

    //继承后根据不同的事件进行不同的处理
    virtual void ConnectEventHandler(int client_sock) = 0;
    virtual void ReadEventHandler(epoll_data_t * user_data) = 0; 
    virtual void WriteEventHandler(epoll_data_t * user_data) = 0;
protected:
    bool EventAdd(int fd, int events)
    {
        struct epoll_event event;
        event.events = events;
        event.data.fd = fd;
        
        if(epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &event) < 0){
            return false;
        }
        return true;
    }
    bool EventDel(int fd)
    {
        if(epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, NULL) < 0){
            return false;
        }
        return true;
    }
    bool EventMod(int fd, int events)
    {
        struct epoll_event event;
        event.events = events;
        event.data.fd = fd;
        if(epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &event) < 0)
        {
            return false;
        }    
        return true;
    }
    void SetNoBlock(int sfd);
protected:
    int port_;        
    int listen_sock_; 
    int epollfd_;     
};

#endif
