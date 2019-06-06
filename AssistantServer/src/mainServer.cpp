#include "head.h"
#include "Worker.h"
#include "processpool.h"
#include "util.h"

int main(int argc, char *argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_colorlogtostderr = true;
    google::SetStderrLogging(google::GLOG_INFO);  //大于指定级别的错误都输出到标准输出
    google::SetLogDestination(google::GLOG_INFO,"../log/mainServer_");
    __C = read_config();
    int res = 0; 
    struct sockaddr_in address;
    bzero(&address, sizeof address);
    address.sin_family = AF_INET;
    inet_pton(AF_INET, __C["MAINSERVER"]["IP"].get<string>().c_str(), &address.sin_addr);
    address.sin_port = htons(__C["MAINSERVER"]["PORT"].get<int>());

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    //reuse addr
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);
    assert(listenfd >= 0);
    
    res = bind(listenfd, (struct sockaddr*)&address, sizeof address);
    /*
    if(res == -1)
    {
        perror("errno:");
    }
    */
    LOG_IF(FATAL, res == -1) << errno;

    res = listen(listenfd, 20);
    LOG_IF(FATAL, res == -1);

    ProcessPool<Worker> *pool = ProcessPool<Worker>::create(listenfd, 8);
    if(pool)
    {
        pool->run();
        delete pool;
    }
    close(listenfd);
    google::ShutdownGoogleLogging();
    return 0;
}