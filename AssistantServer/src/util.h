#ifndef UTIL_H
#define UTIL_H
#include "head.h"
#include "msql.h"


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
json s_login(MSQL *conn, const string &username, const string &password);
//bool s_login(std::shared_ptr<MSQL> conn, const string &username, const string &password);
bool s_register(MSQL *conn, const json &cfg);
bool s_update_info(MSQL *conn, const json &cfg);
//bool s_register(std::shared_ptr<MSQL> conn, const json &cfg);
void addfd(int epollfd, int fd, bool enable_et);

void removefd(int epollfd, int fd);

string get_user_id(MSQL *conn, const string &username);
string get_user_name(MSQL *conn, const string &user_id);


json read_config();

#endif