#ifndef WORKER_H
#define WORKER
#include "head.h"
#include "processpool.h"
#include "util.h"
#include "msql.h"

class Worker
{
private:
    static int m_epollfd;
    int m_sockfd;
    sockaddr_in m_address;
    //char m_buf[1024];
    //std::shared_p<MSQL> conn;
    MSQL *conn;
public:
    Worker();
    ~Worker(){
        if(conn) 
        {
            cout<<"delete conn \n";
            delete conn;
        }
    }
    void init(int epollfd, int sockfd, const sockaddr_in &client_addr);
    void process();
};



#endif