/*
参考《Linux高性能服务器编程》第十五章P289
进程池的实现
*/
#ifndef PROCESSPOOL_H
#define PROCESSPOOL_H
#include "head.h"

/*
描述一个子进程的类
m_pid是目标子进程的PID
m_pipefd是父子进程通信用的管道
*/
class Process
{
public:
    Process() : m_pid(-1) {}

public:
    pid_t m_pid;
    int m_pipefd[2];
};

template<typename T>
class ProcessPool
{
private:
    static const int MAX_PROCESS_NUMBER = 16; //进程池允许的最大子进程数量
    static const int USER_PER_PROCESS = 65536; //每个子进程最多能处理的用户数量
    static const int MAX_EVNET_NUMBER = 10000; //epoll最多能处理的事件数

    int m_process_number; //进程池中的进程总数
    int m_idx; //子进程在池中的序号，从0开始
    int m_epollfd; //每个进程都有一个epoll内核事件表，用m_epollfd标识
    int m_listenfd; //监听socket

    int m_stop; //子进程通过m_stop 决定是否停止运行
    Process *m_sub_process; //保存所有子进程的描述信息
    static ProcessPool<T> *m_instance;

private:
    ProcessPool(int listenfd, int process_number = 8);

    int choose_process(int cur);

    void setup_sig_pipe();
    void run_parent();
    void run_child();

public:
    ProcessPool() = delete; //
    static ProcessPool<T>* create(int listenfd, int process_number = 8)
    {
        if(!m_instance)
        {
            m_instance = new ProcessPool<T>(listenfd, process_number);
        }
        return m_instance;
    }
    ~ProcessPool()
    {
        delete[] m_sub_process;
    }
    void run();

};

template<typename T>
ProcessPool<T>* ProcessPool<T>::m_instance = NULL;

static int sig_pipefd[2]; //用于处理信号的管道，以实现统一事件源，信号管道


static int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK; // 非阻塞
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

static void addfd(int epollfd, int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET; //边缘模式
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

static void removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

static void sig_handler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    send(sig_pipefd[1], reinterpret_cast<char*>(&msg), 1, 0);  
    errno = save_errno;
}

static void addsig(int sig, void(handler) (int), bool restart = true)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof sa);
    sa.sa_handler = handler;
    if(restart)
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    //assert(sigaction(sig, &sa, NULL) != -1);
    LOG_IF(FATAL, sigaction(sig, &sa, NULL) == -1);
}

template<typename T>
ProcessPool<T>::ProcessPool(int listenfd, int process_number)
:m_listenfd(listenfd), m_process_number(process_number), m_idx(-1), m_stop(false)
{
    //assert((process_number > 0) && (process_number <= MAX_PROCESS_NUMBER));
    LOG_IF(FATAL, !((process_number > 0) && (process_number <= MAX_PROCESS_NUMBER)));
    m_sub_process = new Process[process_number];
    //assert(m_sub_process);
    LOG_IF(FATAL, m_sub_process == nullptr);

    //create process_number processes and create the pipe with their father_process
    for(int i = 0; i < process_number; i++)
    {
        int res = socketpair(PF_UNIX, SOCK_STREAM, 0, m_sub_process[i].m_pipefd);
        //assert(res == 0);
        LOG_IF(FATAL, res != 0);
        m_sub_process[i].m_pid = fork();
        //assert(m_sub_process[i].m_pid >= 0);
        LOG_IF(FATAL, m_sub_process[i].m_pid < 0);
        if(m_sub_process[i].m_pid > 0)
        {
            //in father_process
            close(m_sub_process[i].m_pipefd[1]); // close the read port;
            continue;
        }
        else 
        {
            //in child_process
            close(m_sub_process[i].m_pipefd[0]); //close the write port;
            m_idx = i;
            break;
        }
    }
}

//统一事件源
template<typename T>
void ProcessPool<T>::setup_sig_pipe()
{
    m_epollfd = epoll_create(5);
    //assert(m_epollfd != -1);
    LOG_IF(FATAL, m_epollfd == -1);
    int res = socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipefd);
    //assert(res != -1);
    LOG_IF(FATAL, res == -1);

    setnonblocking(sig_pipefd[1]);
    addfd(m_epollfd, sig_pipefd[0]);

    //设置信号处理函数
    addsig(SIGCHLD, sig_handler);
    addsig(SIGTERM, sig_handler);
    addsig(SIGINT, sig_handler);
    addsig(SIGPIPE, sig_handler);
}

template<typename T>
void ProcessPool<T>::run()
{
    if(m_idx != -1)
    {
        run_child();
    }
    else
    {
        run_parent();
    }
}

template<typename T>
void ProcessPool<T>::run_child()
{
    setup_sig_pipe();
    int pipefd = m_sub_process[m_idx].m_pipefd[1]; //与父进程通信的管道，read port
    addfd(m_epollfd, pipefd); //监听来自父进程的消息

    epoll_event events[MAX_EVNET_NUMBER];
    T *users = new T[MAX_PROCESS_NUMBER];
    //assert(users);
    LOG_IF(FATAL, users == nullptr);
    
    int number = 0;
    int res = -1;
    while(!m_stop)
    {
        number = epoll_wait(m_epollfd, events, MAX_EVNET_NUMBER, -1); 
        if((number < 0) && (errno != EINTR))
        {
            kill(getppid(), SIGINT);  //子进程异常，就发信号给父进程结束所有进程
            LOG(FATAL) << "epoll failed, kill all process";
        }
        for(int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            if(sockfd == pipefd && events[i].events & EPOLLIN)
            {
                LOG(INFO) << "received: " << getpid();
                int client = 0;
                res = recv(sockfd, reinterpret_cast<char*>(&client), sizeof client, 0);
                if((res < 0 && errno != EAGAIN) || res == 0)
                {
                    continue;
                }
                else
                {
                    struct sockaddr_in client_address;
                    socklen_t client_addrlength = sizeof client_address;
                    int connfd = accept(m_listenfd, (struct sockaddr*)(&client_address), &client_addrlength);
                    LOG(INFO) <<"client: " << connfd ;
                    if(connfd < 0)
                    {
                        LOG(WARNING) << "errno: " << errno;
                        perror("?");
                        continue;
                    }
                    addfd(m_epollfd, connfd);
                    //模板必须实现init方法，以初始化一个客户连接
                    //这里直接用connfd来索引对象，提高效率
                    users[connfd].init(m_epollfd, connfd, client_address); 
                }
            }
            else if(sockfd == sig_pipefd[0] &&events[i].events & EPOLLIN)
            {
                int sig;
                char signals[1024];
                res = recv(sig_pipefd[0], signals, sizeof signals, 0);
                if(res <= 0)
                {
                    continue;
                }
                else
                {
                    for(int i = 0; i < res; i++)
                    {
                        switch (signals[i])
                        {
                        case SIGCHLD:
                        {
                            pid_t pid;
                            int stat;
                            while((pid = waitpid(-1, &stat, WNOHANG)) >= 0)
                            {
                                continue;
                            }
                            break;
                        }
                        case SIGTERM:
                        case SIGINT:
                        {
                            m_stop = true;
                            break; 
                        }
                        default:
                            break;
                        }
                    }
                }
            }
            else if(events[i].events & EPOLLIN)
            {
                users[sockfd].process();  //客户请求到达，处理请求
            }
            else
            {
                continue;
            }
        }
    }
    delete[] users;
    users = nullptr;
    // close(m_listenfd); //哪个函数创建就由哪个函数close
    close(pipefd);
    close(m_epollfd);
}

template <typename T>
void ProcessPool<T>::run_parent()
{
    setup_sig_pipe();

    //监听m_listenfd;
    addfd(m_epollfd, m_listenfd);
    epoll_event events[MAX_EVNET_NUMBER];
    int sub_process_counter = 0;
    int new_conn = 1;  //提醒子进程有新的连接请求
    int number = 0;
    int res = -1;

    while(!m_stop)
    {
        number = epoll_wait(m_epollfd, events, MAX_EVNET_NUMBER, -1);
        if(number < 0 && errno != EAGAIN)
        {
            LOG(FATAL) << "epoll failed";
        }
        for(int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            if(sockfd == m_listenfd)
            {
                //新的连接到来
                int i = choose_process(sub_process_counter);
                if(m_sub_process[i].m_pid == -1)
                {
                    m_stop = true;
                    break;
                }
                sub_process_counter = (i+1) % m_process_number;
                send(m_sub_process[i].m_pipefd[0], reinterpret_cast<char*>(&new_conn), sizeof new_conn, 0);
                LOG(INFO) << "send req to child: " << i; 
            }
            else if(sockfd == sig_pipefd[0] && events[i].events & EPOLLIN)
            {
                //父进程接收到的信号
                int sig;
                char signals[1024];
                res = recv(sig_pipefd[0], signals, sizeof signals, 0);
                if(res <= 0)
                {
                    continue;
                }
                else
                {
                    for(int i = 0; i < res; i++){
                        switch (signals[i])
                        {
                        case SIGCHLD:
                        {
                            pid_t pid;
                            int stat;
                            int t_cnt = 0; //统计结束的进程数
                            while((pid = waitpid(-1, &stat, WNOHANG)) > 0)
                            {
                                t_cnt++;
                                for(int i = 0; i < m_process_number; i++)
                                {
                                    if(m_sub_process[i].m_pid == pid)
                                    {
                                        LOG(INFO) << "child " << i << "join";
                                        close(m_sub_process[i].m_pipefd[0]);
                                        m_sub_process[i].m_pid = -1;
                                        break; //yijiull 
                                    }
                                }
                            }
                            if(t_cnt == m_process_number)
                            {
                                m_stop = true;
                            }
                            break;
                        }
                        case SIGTERM:
                        case SIGINT:
                        {
                            //父进程终止，杀死所有子进程
                            //TODO 可以更优雅的结束子进程
                            LOG(WARNING) << "kill all the child now";
                            for(int i = 0; i < m_process_number; i++)
                            {
                                int pid = m_sub_process[i].m_pid;
                                if(pid != -1)
                                {
                                    kill(pid, SIGTERM);
                                }
                            }
                            break;
                        }
                        default:
                            break;
                        }
                    }
                }
            }
            else
            {
                continue;
            }
        }
    }
    //close(m_listenfd); //哪个函数创建就由哪个函数close
    close(m_epollfd);
}

//TODO
//这里使用了Round—Robin方式分配一个子进程
//后续可以根据子进程的负载（线程池使用情况）进行分配
template <typename T>
int ProcessPool<T>::choose_process(int cur)
{
    int i = cur;
    cur--;
    while(i != cur)
    {
        if(m_sub_process[i].m_pid != -1)
        {
            break;
        }
        i = (i+1) % m_process_number;
    }
    return i;
}
#endif
