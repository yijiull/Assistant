#ifndef UTIL_H
#define UTIL_H
#include "head.h"

//用来解析文件目录
struct Node
{
    string name;
    vector<Node*> sub_dirs;
};

class Tree
{
private:
    Node *rt;
    bool insert(const string &path);
    void clear(Node *cur);
public:
    Tree();
    ~Tree();
    bool build(const json &files);
    Node* get_root();
    void print();
};

Tree easy_parse(json files);

int setNonblock(int fd);
int unblock_connect(const char* ip, const int port, int time);

json* read_config(const char* config_file);

ssize_t rio_readn(int fd, char *usrbuf, size_t n);
ssize_t rio_writen(int fd, const char *usrbuf, size_t n);

bool send_json(int fd, json &task);
bool receive_json(int epollfd, int sockfd, json &task);
int my_open(string &file);
vector<string> walk(const string &dir); //遍历得到该目录下的所有文件

void files_to_json(const string &file_path, json &msg); //将该目录（文件）下的所有文件信息打包成json

string file_to_socket(const json &cfg, const int sockfd);
string socket_to_file_client(const int sockfd, const int savefd, const size_t length_to_download, json *config);
string socket_to_file_server(const int sockfd, const int savefd, const size_t length_to_download);


//##################### CLIENT ########################
void addfd(int epollfd, int fd, bool enable_et);

void removefd(int epollfd, int fd);
json read_config();
#endif