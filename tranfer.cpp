#include <signal.h>
#include "tranfer.h"

#define SOCKS5PORT 8001


void TranferServer::ConnectEventHandler(int client_sock)
{
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        LOG(WARNING, "socket create error for connect Socks5Server");
        return;
    }
    if(connect(server_sock, (struct sockaddr *)&socks5addr_, sizeof(socks5addr_)) < 0)
    {
        LOG(WARNING, "connect Socks5Server error");
        return;
    }

    //到此说明两边建立连接成功 添加事件

    SetNoBlock(client_sock);
    EventAdd(client_sock, EPOLLIN);

    SetNoBlock(server_sock);
    EventAdd(server_sock, EPOLLIN);

    Connect * con = new Connect;
    con->state_ = FORWARDING;

    con->clientchannel_.fd_ = client_sock;
    fdconnect_map[client_sock] = con;
    con->ref_++;

    con->serverchannel_.fd_ = server_sock;
    fdconnect_map[server_sock] = con;
    con->ref_++;
    //LOG(INFO, "start handler");
}
void TranferServer::ReadEventHandler(epoll_data_t * user_data)
{
    int fd = user_data->fd;
    std::unordered_map<int, Connect*>::iterator it = fdconnect_map.find(fd);
    if(it != fdconnect_map.end())
    {
        Connect * con = it->second;

        //默认发送数据的客户端
        Channel * transmitting_end = &con->clientchannel_;  //发送端
        Channel * receiving_end = &con->serverchannel_;     //接收端

        //当发送端时client时接收端时Socks5Server
        //发送时需要加密，接收时不需要解密
        bool send_encry = true, recv_decrypt = false;

        //转发两边的数据 如果服务器是发送端进行交换
        if(fd == con->serverchannel_.fd_)
        {
            std::swap(transmitting_end, receiving_end);

            //当发送端时server时接收端为自己(TranferServer)
            //发送时不需要加密, 接收时需要解密
            std::swap(send_encry, recv_decrypt);
        }
        Forwarding(con, transmitting_end, receiving_end, send_encry, recv_decrypt);
    }
    else
    {
        LOG(ERROR, "no find connect fd event");
        return;
    }
}

int main ()
{
    signal(SIGPIPE, SIG_IGN);
    TranferServer server(8000, "192.168.0.104", SOCKS5PORT);
    server.Start();
    server.EventLoop(); 
    return 0;
}











