#include "util.h"
#include "mcrypto.h"
string CONFIG = "../.config/config.json";


Tree::Tree()
{
    rt = new Node;
}
void Tree::clear(Node *cur)
{
    if(cur)
    {
        for(int i = 0; i < cur->sub_dirs.size(); i++)
        {
            clear(cur->sub_dirs[i]);
        }
        delete cur;
    }
}
Tree::~Tree()
{
    clear(rt);
}

bool Tree::insert(const string &path)
{
    LOG(INFO) << path;
    size_t st = 0;
    size_t pos = -1;
    Node *cur = rt;
    while((pos = path.find('/', st)) != string::npos)
    {
        string item = path.substr(st, pos - st);
        st = pos + 1;
        bool flag = false;
        for(int i = 0; i < cur->sub_dirs.size(); i++)
        {
            if(cur->sub_dirs[i]->name == item)
            {
                //找到该目录
                cur = cur->sub_dirs[i];
                flag = true;
                break;
            }
        }
        if(!flag)
        {
            //没有找到该目录
            Node *temp = new Node;
            temp->name = item;
            cur->sub_dirs.push_back(temp);
            cur = temp;
        }
    }
    string file = path.substr(st); //文件
    Node *temp = new Node;
    temp->name = file;
    cur->sub_dirs.push_back(temp);
    return true;
}
bool Tree::build(const json &files)
{
    auto temp = files[0]["file-name"].get<string>().substr(8); // "../home/"占8个字符，从这之后开始解析
    int len = temp.find_first_of('/');
    LOG(INFO) << "Parse files: " << temp.substr(0, len);
    int n = files.size();
    rt->name = temp.substr(0, len);
    for(int i = 0; i < n; i++)
    {
        if(!insert(files[i]["file-name"].get<string>().substr(8 + len + 1)))
        //if(!insert(files[i]["file-name"].get<string>().substr(8)))
        {
            return false;
        }
    }
    return true;
}
Node* Tree::get_root()
{
    return rt;
}
void Tree::print()
{
    std::queue<Node*> q;
    q.push(rt);
    while(!q.empty())
    {
        auto temp = q.front();
        q.pop();
        cout<<temp->name<<endl;
        for(int i = 0; i < temp->sub_dirs.size(); i++)
        {
            q.push(temp->sub_dirs[i]);
        }
    }
}

Tree easy_parse(json files)
{
    Tree tree;
    tree.build(files);
    return tree;
}


int setNonblock(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}
void addfd(int epollfd, int fd, bool enable_et)  //enable_et是否启用ET模式
{
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if(enable_et)
    {
        ev.events |= EPOLLET;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    setNonblock(fd);
}

void removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
    cout<<"closed client: "<<fd<<endl;
}


int unblock_connect(const char* ip, const int port, int time)
{
    int res = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof address);
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int fdopt = setNonblock(fd);
    res = connect(fd, (struct sockaddr*)&address, sizeof address);
    if(res == 0) //直接连接成功
    {
        printf("connetct with server imediately\n");
        fcntl(fd, F_SETFL, fdopt);
        return fd;
    }
    else if(errno != EINPROGRESS) //连接没有在进行
    {
        printf("unblcok connect not support\n");
        return -1;
    }
    fd_set writefds;
    struct timeval timeout;
    FD_ZERO(&writefds);
    FD_SET(fd, &writefds);
    timeout.tv_sec = time;
    timeout.tv_usec = 0;

    res = select(fd+1, NULL, &writefds, NULL, &timeout);  //连接成功或者失败都会导致可写事件
    if(res <= 0)  //select 超时或出错
    {
        printf("connect timeout\n");
        close(fd);
        return -1;
    }
    if(!FD_ISSET(fd, &writefds))
    {
        printf("no events on fd found\n");
        close(fd);
        return -1;
    }
    int error = 0;
    socklen_t len = sizeof error;
    if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) //出错
    {
        printf("get socket option failed\n");
        close(fd);
        return -1;
    }
    if(error != 0)  //出错
    {
        printf("connection failed after select with error: %d\n", error);
        close(fd);
        return -1;
    }
    //连接成功
    printf("connect ready after select with the socket: %d\n", fd);
    fcntl(fd, F_SETFL, fdopt);
    return fd;
}

json* read_config(const char *config_file)
{
    json *config = new json;
    std::ifstream in(config_file);
    in >> *config;
    return config;
}

ssize_t rio_readn(int fd, char *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;
    while(nleft > 0)
    {
        if((nread = read(fd, bufp, nleft)) < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
            else if(errno == EINTR)
            {
                nread = 0;
            }
            else
            {
                return -1;
            }
            
        }
        else if(nread == 0)
        {
            break; // EOF
        }
        nleft -= nread;
        bufp += nread;
    }
    return n - nleft;
}

ssize_t rio_writen(int fd, const char *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten;
    const char *bufp = usrbuf;
    while(nleft > 0)
    {
        if((nwritten = write(fd, bufp, nleft)) <= 0) // <=0
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
            else if(errno == EINTR)
            {
                nwritten = 0;
            }
            else
            {
                return -1;
            }
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n - nleft;  //BUG?
}

bool send_json(int fd, json &task)
{
    string str_msg = task.dump();
    cout<<"send: "<<str_msg<<endl;
    ssize_t res = rio_writen(fd, const_cast<char *>(str_msg.c_str()), str_msg.length()); 
    return res == str_msg.length();
}
//ET模式下调用
//TOOD: 待测试
bool receive_json(int epollfd, int sockfd, json &task)
{
    string msg;
    char buf[__C["BUF_SIZE"].get<int>()];
    while(true) //ET模式, 读到EAGAIN || EWOULDBLOCK为止
    {
        memset(buf, 0, sizeof buf);
        int res = recv(sockfd, buf, __C["BUF_SIZE"].get<int>()-1, 0);
        LOG(WARNING) << buf;
        if(res < 0)
        {
            if((errno == EWOULDBLOCK) || (errno == EAGAIN))  //对于非阻塞IO，表示已经读完
            {
                printf("read done\n");
                break;
            }
            removefd(epollfd, sockfd);
            break;
        }
        else if(res == 0)  //服务端不会关闭连接
        {
            cout<<"remote closed the socket\n";

            return true;
        }
        else
        {
            //printf("get %d bytes of content: %s\n", res, buf);
            msg += string(buf, buf + res);
        }
    }
    cout<<"客户端发来的消息：\n";
    cout<<msg<<endl;
    std::stringstream ss(msg);
    ss >> task;
    return false;
}

int my_open(string &file)
{
    size_t pos = 0;
    while((pos = file.find_first_of('/', pos)) != string::npos)
    {
        string subdir = file.substr(0, pos);
        //cout<<"subdir: "<<subdir<<endl;
        int res = mkdir(subdir.c_str(), 0755);
        if(res == -1)
        {
            //已存在
            //cout<<"already exists!\n";
        }
        pos++;
    }
    int fd = open(file.c_str(), O_RDWR | O_CREAT, 0664);
    assert(fd >= 0);
    return fd;
}

vector<string> walk(const string &dir)
{
    vector<string> all_files;
    struct stat file_info;
    int res = stat(dir.c_str(), &file_info);
    if(S_ISREG(file_info.st_mode))
    {
        all_files.push_back(dir);
        return all_files;
    }

    auto dirs = opendir(dir.c_str());
    errno = 0;
    struct dirent *subdir;
    while((subdir = readdir(dirs)))  //BUG? TODO:可重入
    {
        /*
        //# ref: man 7 inode | grep -A 50 "The file type and mode"
        subdir->d_type;  
        */
        string filename = dir + "/" + string(subdir->d_name);
        struct stat file_info;
        int res = stat(filename.c_str(), &file_info);
        if(S_ISDIR(file_info.st_mode))  //目录?
        {
            if(subdir->d_name[0] == '.')
            {
                cout<<filename<<endl;
                continue;
            }
            auto res = walk(filename.c_str());
            for(auto &file: res)
            {
                all_files.emplace_back(std::move(file));
            }
        }
        else if(S_ISREG(file_info.st_mode)) //普通文件?
        {
            all_files.push_back(std::move(filename));
        }
    }
    return all_files;
}
string file_to_socket(const json &cfg, const int sockfd)
{
    /*
    // json格式
        file-name:string
        st: long
        cur-pos: long 
        file-length: size_t
    */
    Hash mm(__C["HASH_TYPE"].get<string>());   

    string file = cfg["file-name"].get<string>();
    int file_fd = open(file.c_str(), O_RDWR);
    off_t off = cfg["st"].get<off_t>() + cfg["cur-pos"].get<off_t>(); //long;
    size_t cnt = cfg["file-length"].get<size_t>() - cfg["cur-pos"].get<off_t>();
    LOG(INFO) << "send:  " << file;
    LOG(INFO) <<"from " << off<<" to "<<off+cnt;
    int res_off = lseek(file_fd, off, SEEK_SET);
    assert(res_off == off);

    int tot = 0;
    char buf[__C["BUF_SIZE"].get<int>()];
    int all = cnt;
    while(cnt)
    {
        int toread = __C["BUF_SIZE"].get<int>() < cnt ? __C["BUF_SIZE"].get<int>() : cnt;
        int res = rio_readn(file_fd, buf, toread);
        if(res == -1)
        {
            LOG(WARNING) << "ERROR!";
            perror("read_n");
            break;
        }
        int temp = rio_writen(sockfd, buf, res);
        assert(temp == res);
        tot += res;
        cnt -= res;
        mm.update(string(buf, buf + res));
    }
    assert(tot == all);
    close(file_fd);
    return mm.hash();
}
void files_to_json(const string &file_path, json &msg)
{
    auto files = walk(file_path);
    int cnt = 0;
    for(auto &file: files)
    {
        struct stat file_info;
        stat(file.c_str(), &file_info);
        json sfile;
        sfile["file-name"] = file;
        sfile["file-length"] = file_info.st_size;
        sfile["cur-pos"] = 0;
        msg[cnt++] = sfile;
    }
}

string socket_to_file_server(const int sockfd, const int savefd, const size_t length_to_download)
{
    Hash mm(__C["HASH_TYPE"].get<string>());
    char buf[__C["BUF_SIZE"].get<int>()];
    int tot = 0;
    int n;
    int cnt = length_to_download;
    while(cnt)
    {
        int toread = __C["BUF_SIZE"].get<int>() < cnt ? __C["BUF_SIZE"].get<int>() : cnt;
        int res = rio_readn(sockfd, buf, toread);
        if(res == -1)
        {
            cout<<"ERROR\n";
            perror("read_n");
            break;
        }
        int s_write = rio_writen(savefd, buf, res);
        assert(s_write == res);
        tot += res;
        cnt -= res;
        mm.update(string(buf, buf + n));
    }
    assert(length_to_download == tot);
    return mm.hash();
}
string socket_to_file_client(const int sockfd, const int savefd, const size_t length_to_download, json *config)
{

    Hash mm(__C["HASH_TYPE"].get<string>());
    char buf[__C["BUF_SIZE"].get<int>()];
    int tot = 0;
    int n;
    int cnt = length_to_download;
    while(cnt)
    {
        int toread = __C["BUF_SIZE"].get<int>() < cnt ? __C["BUF_SIZE"].get<int>() : cnt;
        int res = rio_readn(sockfd, buf, toread);
        if(res == -1)
        {
            cout<<"ERROR\n";
            perror("read_n");
            break;
        }
        int s_write = rio_writen(savefd, buf, res);
        assert(s_write == res);
        tot += res;
        cnt -= res;
        mm.update(string(buf, buf + res));
        (*config)["cur-pos"] = (*config)["cur-pos"].get<int>() + res;
    }
    assert(length_to_download == tot);
    return mm.hash();
}

json read_config()
{
    std::ifstream in(CONFIG);
    json res;
    in >> res;
    return res;
}