#include "head.h"
#include "util.h"


void et(epoll_event* events, int number, int epollfd, int listenfd)
{
    char buf[__C["BUF_SIZE"].get<int>()];
    for(int i = 0; i < number; i++)
    {
        string msg;  //收到的消息
        int sockfd = events[i].data.fd;
        if(sockfd == listenfd)  //处理新到来的连接
        {
            LOG(INFO) << "new client";
            struct sockaddr_in client_address;
            socklen_t len = sizeof client_address;
            while(1) //IMPORTANT:需要一直accept,否则会错过客户端的connect
            {
                int connfd = accept(listenfd, (struct sockaddr*)&client_address, &len); 
                if(connfd == -1)
                {
                    if(errno == EAGAIN || errno == EWOULDBLOCK)  //本次触发的connect已经处理完
                    {
                        break;
                    }
                }
                else
                {
                    addfd(epollfd, connfd, true);
                }
            } 

        }
        else if(events[i].events == EPOLLIN) //处理客户端的数据
        {
            bool closed = false; //对方是否关闭了连接
            while(true) //ET模式, 读到EAGAIN || EWOULDBLOCK为止
            {
                memset(buf, 0, sizeof buf);
                int res = recv(sockfd, buf, __C["BUF_SIZE"].get<int>()-1, 0);
                if(res < 0)
                {
                    if((errno == EWOULDBLOCK) || (errno == EAGAIN))  //对于非阻塞IO，表示已经读完
                    {
                        printf("read later\n");
                        break;
                    }
                    removefd(epollfd, sockfd);
                    break;
                }
                else if(res == 0)  //检测到客户端关闭了连接
                {
                    closed = true;
                    break;
                }
                else
                {
                    //printf("get %d bytes of content: %s\n", res, buf);
                    msg += string(buf, buf + res);
                }
            }
            if(closed) //检测到客户端关闭了连接，服务端也关闭
            {
                removefd(epollfd, sockfd);  //TODO：移除epoll对该描述符的监听
                continue; //处理下一个事件
            }
            LOG(INFO) << "MSG:  "<<msg;
            std::stringstream ss(msg);
            json cfg;
            ss >> cfg;

            auto type = cfg["type"].get<MSG_TYPE>();
            switch (type)
            {
            case QUERY_FILE_OR_DIR: //请求需要下载的文件信息
            {
                LOG(INFO) << "QUERY_FILE_OR_DIR";
                string file_path = cfg["file"].get<string>();
                //遍历目录，将目录下的文件列表传给对方
                json msg;
                file_path = "../home/" + file_path;
                files_to_json(file_path, msg);
                assert(send_json(sockfd, msg));
                LOG(INFO) << msg.dump();
                //close(sockfd);  //发完就关闭连接，对方收到EOF后开始下载
                removefd(epollfd, sockfd);
                break;
            }
            case DOWNLOAD_FILE:  //发送下载请求
            {
                LOG(INFO) << "DOWNLOAD_FILE";
                string hash = file_to_socket(cfg, sockfd);
                int res = rio_writen(sockfd, hash.c_str(), hash.size());
                assert(res == hash.size());
                //close(sockfd);  //不能关闭，可能需要下载多个文件
                break; 
            }
            case UPLOAD_FILE:
            {
                LOG(INFO) << "UPLOAD_FILE";
                std::stringstream ss(msg);
                json files;
                ss >> files;

                //json ack;
                //ack["type"] = "ack";
                //send_json(sockfd, ack);
                char c = '1';
                write(sockfd, &c, 1);  //ACK
                
                //start 
                int thread_number = files["info"].size();
                assert(thread_number == 1); //单线程上传

                string dst = files["dst"].get<string>();
                string home_dir = files["home_dir"].get<string>();
                size_t pos = home_dir.find_last_of('/');
                int file_number = files["info"][0].size();
                json task = files["info"][0];
                LOG(INFO) << "dst: " << dst;
                LOG(INFO) << "file_number: " << file_number;
                for(int i = 0; i < file_number; i++)
                {
                    string file_name = task[i]["file-name"].get<string>().substr(pos + 1);
                    __off64_t file_off = task[i]["st"].get<long>() + task[i]["cur-pos"].get<long>(); // 文件偏移
                    size_t length_to_download = task[i]["file-length"].get<size_t>(); //需要下载的文件长度
                    file_name = "../home/" + dst + "/" + file_name;
                    int savefd = my_open(file_name); 
                    //cout<<"file: "<<savefd<<endl;
                    LOG(INFO) << i << " " << "filename: " << file_name << " length: " << length_to_download;

                    int res_off = lseek(savefd, file_off, SEEK_SET);
                    assert(res_off == file_off);
                    socket_to_file_server(sockfd, savefd, length_to_download);
                    //close(savefd);
                    //removefd(epollfd, sockfd);
                }
                //close(sockfd); //TODO, remove sockfd from epollfd;
                removefd(epollfd, sockfd);

                break;
            }
            case DELETE_FILE:
            {
                LOG(INFO) << "DELETE_FILE";
                auto path = "../home/" + cfg["path"].get<string>();
                LOG(WARNING) << path;
                auto res = remove(path.c_str());
                if(res == 0)
                {
                    //删除成功
                    LOG(INFO) << "删除成功";
                }
                else
                {
                    LOG(INFO) << "删除失败";
                }
                
                /*
                auto res = walk(path);
                for(auto &file : res)
                {
                    
                }
                */
                removefd(epollfd, sockfd);

                break;
            }
            case close_socket: //关闭连接, 没用
            {
                //close(sockfd);
                removefd(epollfd, sockfd);
                break;
            }
            default:
                break;
            }
        }
        else  // error?
        {
            LOG(WARNING) << "something unexpected happened\n";
        }
    }
}


//int main(int argc, char* argv[])
int main(int argc, char* argv[])
{
    /*
    if(argc <= 2)
    {
        printf("usage: ip, port\n");
        return 1;
    }
    int res = 0;
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    */ 
    __C = read_config();
    google::InitGoogleLogging(argv[0]);
    FLAGS_colorlogtostderr = true;
    google::SetStderrLogging(google::GLOG_INFO);  //大于指定级别的错误都输出到标准输出
    google::SetLogDestination(google::GLOG_INFO,"../log/FileServer_");
    int res = 0;
    string ip = __C["FILESERVER"]["IP"].get<string>();
    const int port = __C["FILESERVER"]["PORT"].get<int>();
    struct sockaddr_in address;
    bzero(&address, sizeof address);
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    //reuse addr
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);
    assert(listenfd >= 0);
    
    res = bind(listenfd, (struct sockaddr*)&address, sizeof address);
    assert(res != -1);

    res = listen(listenfd, 20);
    assert(res != -1);

    epoll_event events[__C["MAX_EVENT"].get<int>()];
    int epollfd = epoll_create(5); //非0即可
    assert(epollfd != -1);

    addfd(epollfd, listenfd, true);

    while(true)
    {
        int res = epoll_wait(epollfd, events, __C["MAX_EVENT"].get<int>(), -1); //-1代表无限等，直至返回
        LOG(INFO) << "epoll_wait_cnt" << res;
        if(res < 0)
        {
            if(errno == EINTR){
                LOG(WARNING) << "EINTR";
                continue;
            }
            LOG(ERROR) << "epoll failed";
            perror("fuck");
            break;
        }
        et(events, res, epollfd, listenfd);
    }
    //close(listenfd);
    removefd(epollfd, listenfd); //??
    close(epollfd);
    google::ShutdownGoogleLogging();
    return 0;
}